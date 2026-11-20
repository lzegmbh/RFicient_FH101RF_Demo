# Driver API

The driver is plain C11 without any RTOS dependency. Two files, no other includes:

| File | Content |
|---|---|
| `wurx.h` | API, register map, enums and the preset declarations |
| `wurx.c` | Implementation |

## Platform hooks

The only platform dependency are three functions the application has to provide. They are
declared **weak**, so a sketch that only references a preset can be linked without implementing
SPI at all:

```c
void    wurx_write_reg(uint8_t addr, uint8_t value);
uint8_t wurx_read_reg(uint8_t addr);
void    wurx_reset_register(void);   /* weak, empty default */
```

In the sketches these live in `fh101rf_platform.cpp`:

```cpp
static const SPISettings fh101rfSpiSettings(FH101RF_SPI_HZ, MSBFIRST, SPI_MODE0);

extern "C" void wurx_write_reg(uint8_t addr, uint8_t val)
{
    SPI.beginTransaction(fh101rfSpiSettings);
    digitalWrite(FH101RF_CS_PIN, LOW);
    SPI.transfer((uint8_t)(addr & 0x7F));   /* bit 7 = 0 -> write */
    SPI.transfer(val);
    digitalWrite(FH101RF_CS_PIN, HIGH);
    SPI.endTransaction();
}

extern "C" uint8_t wurx_read_reg(uint8_t addr)
{
    uint8_t val;

    SPI.beginTransaction(fh101rfSpiSettings);
    digitalWrite(FH101RF_CS_PIN, LOW);
    SPI.transfer((uint8_t)(addr | 0x80));   /* bit 7 = 1 -> read */
    val = SPI.transfer((uint8_t)0x00);      /* dummy byte clocks the value out */
    digitalWrite(FH101RF_CS_PIN, HIGH);
    SPI.endTransaction();

    return val;
}
```

> **Do not include `wurx.h` in the platform file.** The header declares the hooks with
> `__attribute__((weak))`. Including it would make your definitions weak as well and the driver's
> weak defaults would win. Without the include they are strong.

The SPI protocol is mode 0, MSB first, max. 10 MHz, CS_N active low. Bit 7 of the header byte
selects the direction: **1 = read, 0 = write**. The RFM69HCW on the same bus uses the opposite
convention (1 = write), which is why a stray access can never be harmless — see
[Wiring](wiring).

## Initialize the receiver

```c
if(wurx_init(wurx_conf_868_1k_32k_idm2slow_allbranch_dout1))
{
    /* receiverGood: xtal ok, LO cal done, offset cal done */
}
```

`wurx_init()` runs crystal start-up, LO calibration and comparator offset calibration, each with
a bounded timeout. It returns non-zero on success and takes typically about 1 s.

It only returns the overall status. Details are read back through the status registers:

| Register | Name | Expected |
|---|---|---|
| `0x7F` | IC version | `0x41` |
| `0x77` | `XTAL_GOOD` | `0x1` |
| `0x06` | `CALIB_STATUS` | `0x0` |
| — | `ACTUAL_BANDSELECT` | `0x2` for the 868 MHz path |
| — | `BAND_BRANCH_CTRL` | `0x27` with all branches |
| — | `IRQ_SELECT` | `0x20` for `WURX_IDMANDSLOW_IRQ` |

## Handle an interrupt

The ISR writes the event data directly into the entry the caller passes in:

```c
void irq_handler(void)
{
    wurx_irq_event_cont_store_t *store = wurx_get_irq_event_array();

    if(store->wurx_irqevent_store_counter < MAXLISTENTRIES)
    {
        wurx_irq_handler(&store->wurx_irqevent_store[store->wurx_irqevent_store_counter]);
        store->wurx_irqevent_store_counter++;
    }
}
```

The caller is responsible for the counter bound — `wurx_irq_handler()` does not check it. Call
`wurx_clear_irq_count()` after the entries were processed.

The event store is optional and only compiled in with `WURXIRQSTORE`:

```
build_flags = -DWURXIRQSTORE
```

Read the events back with `wurx_get_irq_event_last()` and `wurx_get_irq_event_counter()`.

## Telegram generator (separate library)

`FH101RFTX/src/wurx_telegram.c` is independent of the receiver driver and has no platform
dependency at all:

```c
#include <wurx_telegram.h>

/* generate a telegram, the buffer is provided by the library */
uint16_t wurx_tg_generate(uint16_t id, uint8_t size, const uint8_t *data);
const uint8_t *wurx_tg_get_telegram(void);

/* generate a telegram into an own buffer */
uint16_t wurx_tg_build(uint16_t id, uint8_t size, const uint8_t *data,
                       uint8_t *telegram, uint16_t telegramSize);

/* helpers */
uint16_t wurx_tg_chips(uint8_t size);   /* number of chips */
uint16_t wurx_tg_size(uint8_t size);    /* number of bytes */
uint8_t  wurx_tg_get_chip(const uint8_t *telegram, uint16_t chipIndex);
```

- `size == 0` → only the ID is transmitted (196 bytes / 1568 chips)
- `data == NULL` is only allowed with `size == 0`
- return value `0` means error (buffer too small, `data` NULL with `size > 0`)

Example:

```c
uint8_t payload[2] = {0xA5, 0x5A};

wurx_tg_generate(0x7DA8, 0, NULL);          /* ID only */
uint16_t len = wurx_tg_generate(0x7DA8, 2, payload);  /* ID + 2 bytes */
```

The chips then have to be clocked out at 33096 Hz — see [Signal format](signal-format). The
reference implementation in the README uses **absolute deadlines** instead of
`delayMicroseconds()` per chip, so the mean chip rate stays exact.

## Reference implementation for an external OOK transmitter

If you key an external OOK module through a GPIO instead of using the RFM69, this pattern works
on AVR and SAMD:

```c
static uint32_t txSendTelegram(const uint8_t *telegram, uint16_t chips)
{
    uint32_t start = micros();
    uint32_t next = start;
    uint32_t fraction = 0;

    for (uint16_t chip = 0; chip < chips; chip++)
    {
        while ((int32_t)(micros() - next) < 0) { }   /* absolute deadline */

        txCarrier(wurx_tg_get_chip(telegram, chip)); /* 1 = RF on, 0 = RF off */

        fraction += TX_CHIP_PERIOD_Q16;              /* 30.215 µs in Q16 */
        next += (fraction >> 16);
        fraction &= 0xFFFFUL;
    }
    txCarrier(0);

    return micros() - start;
}
```

Measured 33094.6 Hz with the 4 µs resolution of `micros()` on AVR, error −0.004 %. The unsigned
subtraction keeps the logic correct at the `micros()` overflow.
