# FH101RFTX – FH101RF wake-up telegram generator

Arduino library that generates the OOK chip stream of an **RFicient® FH101RF**
wake-up telegram. The generated bit stream can be transmitted by any
OOK-capable transmitter (transceiver in OOK mode or a simple OOK module) and
matches the 868 MHz preset of the `FH101RFRX` sketch, respectively
`wurx_conf_868_1k_32k_idm2slow_allbranch_dout1` of the driver.

## Telegram structure

According to datasheet section 5.6 (figure 6), table 16 and table 23:

| Section | Transmit rate | Length | Content |
|---|---|---|---|
| 1. Preamble | 1 kbit/s (LDR) | 32 bit = 32 ms | CodeA `0x6ACD674F`, every bit spread over 32 chips |
| 2. Pause | — | 32 chips = 0.98 ms | Transmitter pause, receiver switches to HDR (TP1 = 5.371 ms max.) |
| 3. ID block | 32 kbit/s (HDR) | 16 bit = 15.6 ms | ID MSB first, `CodeA` = 0, `CodeB` = 1 |
| 4. Payload | 32 kbit/s (HDR) | `size` × 8 bit | User data MSB first, same encoding |

- **CodeA** (logical 0): `0x6A 0xCD 0x67 0x4F` (mlsA, datasheet table 23)
- **CodeB** (logical 1): `0x6D 0x38 0x97 0x73` (mlsB, datasheet table 23)

The preamble is transmitted at 1 kbit/s, the data at 32 kbit/s. Because the chip
stream runs continuously at 32.768 kchip/s, every preamble bit is repeated 32
times (spreading factor `WURX_TG_PREAMBLE_SPREAD = 32`).

The generated buffer is bit-packed: bit 7 of byte 0 is the first chip.
Chip = 1 means RF on, chip = 0 means RF off.

## API

```c
#include <wurx_telegram.h>

/* generate a telegram, the buffer is provided by the library */
uint16_t wurx_tg_generate(uint16_t id, uint8_t size, const uint8_t *data);
const uint8_t *wurx_tg_get_telegram(void);

/* generate a telegram into an own buffer */
uint16_t wurx_tg_build(uint16_t id, uint8_t size, const uint8_t *data,
                       uint8_t *telegram, uint16_t telegramSize);

/* helper functions */
uint16_t wurx_tg_chips(uint8_t size);   /* number of chips */
uint16_t wurx_tg_size(uint8_t size);    /* number of bytes */
uint8_t  wurx_tg_get_chip(const uint8_t *telegram, uint16_t chipIndex);
```

- `size == 0` → only the ID is transmitted (telegram 196 bytes / 1568 chips).
- `data == NULL` is only allowed with `size == 0`.
- Return value `0` means error (buffer too small, `data` NULL with `size > 0`).

Example:

```c
uint8_t payload[2] = {0xA5, 0x5A};

/* ID 0x7DA8 only */
wurx_tg_generate(0x7DA8, 0, NULL);

/* ID + 2 bytes of user data */
uint16_t len = wurx_tg_generate(0x7DA8, 2, payload);
```

## Transmitting

The chips have to be clocked at `WURX_TG_TX_CHIP_RATE_HZ` = **33096 Hz**.
According to datasheet section 5.8 that is 1 % more than the sampling rate of the
receiver (32768 Hz), i.e. `fTX = 1.01 × 32768`.

The example `examples/WurxTelegramSendRfm69` handles the transmission with the
on-board RFM69HCW: the chip clocks the chip stream itself from its crystal.

If an external OOK transmitter is to be keyed through a GPIO instead, the
following reference implementation can be used — it waits on **absolute
deadlines** instead of calling `delayMicroseconds()` per chip:

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

Instead of `delayMicroseconds(30)` per chip this waits on **absolute deadlines**.
That way the latencies of `micros()` and `digitalWrite()` cancel out and the mean
chip rate stays exact (measured 33094.6 Hz at the 4 µs resolution of `micros()`
on AVR, error −0.004 %). Individual edges jitter by a few µs, which the
correlator of the FH101RF tolerates over the 32-chip sequence. The unsigned
subtraction keeps the logic correct at the `micros()` overflow.

Important: the transmitter has to deliver true OOK modulation, i.e. radiate
below −60 dBm (better −80 dBm) in the low level. The data rate of the
transmitter has to stay within tolerance, otherwise the receiver detects no
chips.

## Timing

| Telegram | Chips | Duration at 33096 Hz |
|---|---|---|
| ID only | 1568 | 47.4 ms |
| ID + 1 byte | 1824 | 55.1 ms |
| ID + 2 bytes | 2080 | 62.9 ms |
| ID + 6 bytes | 3104 | 93.8 ms |

After the telegram the receiver has to fall back to LDR (TP2 = 1.53 ms) before
the next telegram starts.

## Buffer size

`wurx_tg_generate()` uses an internal buffer of size `WURX_TG_BUFFER_SIZE`
(default 388 bytes = ID + 6 bytes payload). For longer payloads increase
`WURX_TG_BUFFER_SIZE` or use `wurx_tg_build()` with its own buffer. Space
required: `196 + 32 × size` bytes.

The receiver FIFO holds only 16 bit = 2 bytes by default (register 0x56
`FIFO_LENGTH`, values 16/24/32/40 bit). Longer payloads can be transmitted, but
the receiver only captures them up to the configured FIFO length.

## Files

| File | Content |
|---|---|
| `src/wurx_telegram.h` | API, constants, telegram documentation |
| `src/wurx_telegram.c` | Implementation (plain C, usable outside Arduino as well) |
| `examples/WurxTelegramDump` | Demo: generate telegrams, verify them, print them as hexdump |
| `examples/WurxTelegramSendRfm69` | Demo: send a telegram with the RFM69HCW in OOK packet mode |
| `library.properties` | Arduino library metadata |

## Examples

Both examples are **complete, ready-to-use Arduino sketches**: they contain all
required sources, compile and upload unchanged and need no additional libraries.
`WurxTelegramDump` even runs without any radio hardware.

### WurxTelegramDump

`examples/WurxTelegramDump` generates an ID-only and an ID+payload telegram,
prints size, chip count, duration and hexdump and **decodes the result back**
(preamble, ID, payload), so the serial output proves the correctness directly:

```
--- ID only ---
ID                 : 0x7DA8, payload 0 byte(s)
telegram size      : 196 byte(s)
chips              : 1568, duration at 33096 Hz: 47 ms
preamble           : 32 bit at 1024 bit/s, spread x32
verify             : OK
```

### WurxTelegramSendRfm69

`examples/WurxTelegramSendRfm69` sends the same telegram with the **on-board
RFM69HCW** (SX1231H) of the Adafruit Feather M0 RFM69HCW. The chip runs in
**OOK packet mode with unlimited length packet format**: the bytes are written
into the 66 byte FIFO and the chip clocks them out bit by bit (MSB first) at
the rate set in `RegBitrate`. The chip stream is therefore as accurate as the
module crystal — no bit-banging in the main program.

Hardware (on-board module, no additional wiring):

| Signal | Feather M0 RFM69HCW |
|---|---|
| CS | D8 |
| RESET | D4 |
| SCK / MOSI / MISO | D24 / D23 / D22 (hardware SPI) |
| DIO0 | D3 (not used) |

Configuration (`rfm69_ook.cpp`):

| Register | Address | Value | Meaning |
|---|---|---|---|
| `RegOpMode` | `0x01` | `0x04` / `0x0C` | Standby / TX |
| `RegDataModul` | `0x02` | `0x08` | DataMode `00` = **packet mode**, OOK, no shaping |
| `RegBitrate` | `0x03/0x04` | `0x03C7` = 967 | 32 MHz / 967 = **33092 Hz** (1.0099 × 32768 Hz) |
| `RegFrf` | `0x07..0x09` | `0xD91999` | **868.4 MHz** (868 399 963.4 Hz) |
| `RegPreambleMsb/Lsb` | `0x2C/0x2D` | `0x0000` | no additional preamble (minimum) |
| `RegSyncConfig` | `0x2E` | `0x00` | sync word off |
| `RegPacketConfig1` | `0x37` | `0x00` | fixed length, **no CRC**, no DC-free, no address filtering |
| `RegPacketConfig2` | `0x3D` | `0x00` | no AES, no auto RX restart |
| `RegPayloadLength` | `0x38` | `0x00` | **unlimited length packet format** — required so the FIFO content reaches the modulator unchanged |
| `RegFifoThresh` | `0x3C` | `0x80` | TX starts as soon as the FIFO is not empty |
| `RegPaLevel` | `0x11` | `0x52` | PA_BOOST (PA1) active, **0 dBm** |
| `RegPaRamp` | `0x12` | `0x0F` | shortest ramp (10 µs), no shaping — maximum bandwidth |
| `RegOcp` | `0x13` | `0x1A` | over-current protection on, 95 mA |
| `RegDioMapping2` | `0x26` | `0x07` | clock out off (no spurious emission) |

> **Why packet mode and not continuous mode?**
> SX1231H datasheet, section 5.4.1: *“in Continuous mode the NRZ data to
> (from) the (de)modulator is directly accessed by the uC on the bidirectional
> DIO2/DATA pin. **The FIFO and packet handler are thus inactive.**”*
> With `RegDataModul = 0x48` (continuous with bit sync) the FIFO is therefore
> completely switched off: every FIFO write is discarded, the modulator never
> sees data and the PA transmits a **flat, unmodulated carrier**. On a
> spectrum analyser this shows as a signal with 4.4 % envelope swing instead
> of about 90 %.
>
> `PacketFormat = 0` + `PayloadLength = 0x00` selects the **unlimited length
> packet format** (datasheet 5.5.2.3). With `PreambleLength = 0` and
> `SyncOn = 0` the packet handler appends neither preamble nor sync word, so the
> FIFO bytes go into the modulator 1:1.

Sequence in `rfm69_ook_send()`: clear flags → `RegOpMode = TX` → wait for
`PllLock` (the time measurement only starts afterwards, so the settling time
does not distort the result) → fill the FIFO while `FifoFull` is not set → wait
until the FIFO has drained → keep TX active for the duration computed from the
bit rate (`bytes × 8 / bitrate`) → back to standby. In packet mode the SX1231
drains the FIFO at the real bit rate, so `FifoNotEmpty` is a usable measure for
the end of the transmission.

Configuration through defines at the top of the sketch:

| Define | Default | Meaning |
|---|---|---|
| `TX_FREQ_HZ` | `868400000` | Carrier frequency |
| `TX_REPEAT_MS` | `2000` | Transmit interval, `0` = only on `s` |
| `TX_ID` | `WURX_TG_DEFAULT_ID` | Wake-up ID (`0x7DA8`) |
| `RFM69_CS_PIN` | `8` | Chip select |
| `RFM69_RESET_PIN` | `4` | Reset, `-1` = not connected |
| `RFM69_TX_POWER_DBM` | `0` | Transmit power in dBm, −18 … +13 (PA1) |
| `RFM69_PA_LEVEL` | `0x52` | derived from it: PA1 on \| (dBm + 18) |
| `RFM69_PARAMP` | `0x0F` | PA ramp, `0x0F` = 10 µs (shortest) |

Serial output:

```
==================================================
 FH101RF wake-up telegram sender, RFM69HCW OOK
==================================================
carrier frequency  : 868399963 Hz (868399 kHz)
chip rate          : 33092 Hz (RegBitrate = 967)
TX power           : 0 dBm (PA_BOOST / PA1)
ID                 : 0x7DA8
payload            : 2 byte(s): AF FE
telegram           : 260 byte(s), 2080 chips
repeat interval    : 2000 ms

ready: 's' send one telegram, 't' repeat on, 'T' repeat off

sent: ID 0x7DA8, 2 payload byte(s), 260 byte(s), 62855 us
```

Notes on this example:

- 868.4 MHz is 100 kHz above the center frequency of the 868 MHz SAW filter on
  the FH101RFBOB (868.3 MHz) and therefore safely inside the passband. A carrier
  at 868.8 MHz is 500 kHz away, which can mean too much attenuation depending
  on the filter bandwidth.
- The SX1231 is specified for OOK operation up to about 32.768 kbit/s; at
  33092 Hz it sits exactly on the 1 % offset required for the FH101RF.
- The FIFO holds 66 bytes, which is 15.9 ms at 33092 Hz. A 260 byte telegram
  (62.9 ms) is therefore refilled in several blocks. The refill path (read
  `RegIrqFlags2`, write a byte to `RegFifo`) takes only a few microseconds and
  is thus far below the byte time of 241 µs, so the FIFO does not run empty.
- **Important:** in continuous mode the SX1231 does *not* drain the FIFO at the
  symbol rate. It copies the FIFO content into the modulator buffer within
  about one byte time, so `FifoNotEmpty` returns to 0 almost immediately.
  Measurements gave `duration(FIFO empty) = fixed share + one byte time`,
  independent of the payload length, and an unmodulated carrier went out on the
  air. According to datasheet 5.4.1 the FIFO is completely inactive in this
  mode. In packet mode the drain time scales with
  `1/RegBitrate` (measured: 46768 µs at 967, 93467 µs at 1934, 225740 µs at
  4835, 23478 µs at 484) and the envelope shows 93 % swing instead of 4.4 %.
- `rfm69_ook_send()` opens **one** SPI transaction for the whole transmission.
  On SAMD every `beginTransaction()` internally runs `config()` (SERCOM
  disable/init/clock/enable); with one transaction per byte the refill loop is
  slower than the byte time and the FIFO runs empty.
- The library generates the chip stream byte-wise with 1 = RF on. The RFM69 also
  transmits a data bit 1 as carrier on, so the polarity matches.
- For the Feather M0 RFM69HCW there is no dedicated board variant in the
  installed cores; `adafruit:samd:adafruit_feather_m0` works because the radio
  module is addressed exclusively through CS/SPI/reset.

### PA ramp: no “off”, only 10 µs

The SX1231 **cannot switch off** the PA ramp. `RegPaRamp` knows only 16 times,
and the scale runs backwards — smaller values mean *longer* ramps:

| Value | Ramp | | Value | Ramp |
|---|---|---|---|---|
| `0x00` | 3400 µs | | `0x08` | 50 µs |
| `0x01` | 2000 µs | | `0x09` | 40 µs (reset) |
| `0x02` | 1000 µs | | `0x0A` | 31 µs |
| `0x03` | 500 µs | | `0x0B` | 25 µs |
| `0x04` | 250 µs | | `0x0C` | 20 µs |
| `0x05` | 125 µs | | `0x0D` | 15 µs |
| `0x06` | 100 µs | | `0x0E` | 12 µs |
| `0x07` | 62 µs | | **`0x0F`** | **10 µs** |

`0x0F` is therefore the maximum bandwidth the hardware offers. `0x00` would be
the slowest ramp (3.4 ms) and would destroy the telegram completely — which is
why the header aborts with an `#error` for values below `0x0A`.

In addition the `ModulationShaping` field in `RegDataModul` is set to `00`
(`0x08`), so **no** Gaussian or raised-cosine shaping is applied. More bandwidth
is not possible with this chip.

**Compromise at 33092 chip/s:** one chip lasts 30.2 µs, so the 10 µs ramp
already occupies a third of it. Rise and fall together cost around 20 µs during
which the PA is not at full power. That cannot be avoided; anyone who needs more
energy per chip has to raise the transmit power (`RFM69_TX_POWER_DBM` up to
`13`).

If the sketch reports `RFM69HCW not found`, it prints the version actually
read, the SPI clock used and the result of a register write/read test:

| Output | Meaning |
|---|---|
| `RegVersion : 0x24` | Module detected, everything fine |
| `RegVersion : 0x00` + `register test FAILED` | SPI bus delivers no data — wrong board, wrong pins or no supply |
| `RegVersion : 0x00` + `register test OK` | Bus works, but the part is not an RFM69 (or is held in reset) |
| `RegVersion : 0x12` | it is an SX127x/LoRa module, not an RFM69 |

`rfm69_ook_begin()` starts the SPI bus itself (`SPI.begin()`), pulses RESET and
reads `RegVersion` again with a descending SPI clock (8 MHz, 4 MHz, 1 MHz,
250 kHz). The `SPI.begin()` call is required: `SPI.beginTransaction()` alone
does not enable the SERCOM peripheral on SAMD, so all transfers would read
`0x00`.

### RESET is active HIGH — a common pitfall

The RESET signal of the RFM69 (D4) is **active HIGH**. As long as D4 is HIGH the
chip still answers SPI **read** accesses (it then returns the reset defaults),
but **every write access is silently ignored**.

This shows up in a very confusing way:

- `RegVersion` reads `0x24` correctly, the module counts as “detected”.
- A write/read test on a register fails (`0x00` instead of `0xA5`).
- A register dump shows nothing but reset defaults
  (`RegBitrate = 0x1A0B`, `RegFrf = 0x000000`, `RegPaLevel = 0x9F` …).
- The transmit duration is far too short, because the bit rate was never
  programmed.

Proven with bit-banged SPI on the same board:

| D4 | Write access |
|---|---|
| LOW | works |
| HIGH | is ignored (reading still works) |
| open (input) | works, the module pulls the pin down itself |

`rfm69Reset()` therefore pulses D4 briefly HIGH and leaves it **LOW**
afterwards.

Diagnostic commands of the example (through the serial console):

| Key | Function |
|---|---|
| `s` | send one telegram |
| `t` / `T` | telegram repetition on / off |

Without a key press the sketch sends the telegram every `TX_REPEAT_MS`
(default 2000 ms).

### Compiling

```bash
arduino-cli compile --fqbn arduino:avr:uno --library FH101RFTX FH101RFTX/examples/WurxTelegramDump
arduino-cli compile --fqbn adafruit:samd:adafruit_feather_m0 --library FH101RFTX FH101RFTX/examples/WurxTelegramSendRfm69
```

In the Arduino IDE the library has to be installed (copy or link the folder
`FH101RFTX` into `<sketchbook>/libraries/`), otherwise the compiler does not
find `wurx_telegram.h`.

Note: the example folders contain copies of `wurx_telegram.c` and
`wurx_telegram.h`, so they also build without the library being installed. A
copy takes precedence over the library as long as it sits in the sketch
folder. The copies are kept identical to `src/` by the CI check
`.github/workflows/check-copies.yml`.

## Verification

The implementation was checked against the examples from the datasheet:

| ID | expected sequence (fast) | Result |
|---|---|---|
| `0xB000` | `BABB AAAA AAAA AAAA` | OK |
| `0x032F` (broadcast) | `AAAA AABB AABA BBBB` | OK |

In addition a round-trip test covers preamble spreading, pause, ID and payload
decoding, lengths and error cases.

## Notes

- After an ID match the receiver only takes as many bits into the FIFO as
  `FIFO_LENGTH` (register 0x56) allows (default 16 bit = 2 bytes). For longer
  payloads `FIFO_LENGTH` has to be adapted in the receiver.
- The ID check in the receiver is done through `IDM_CTRL` (register 0x38):
  individual ID, group ID (high byte) or broadcast `0x032F`.
- For group wake-up the group ID is transmitted as the high byte of the 16-bit
  ID, the individual ID sits in the low byte.
- The transmitter has to send MSB first — the library already generates the
  sequence in that order.
- `WurxTelegramSendRfm69` shows the hardware variant: a transceiver in OOK
  packet mode (RFM69HCW) takes the same chip stream and clocks it itself from
  the module crystal.
- While transmitting, the receiver stays tuned to the transmit frequency — in a
  single-band setup with one antenna path, care must be taken that transmitter
  and receiver are sufficiently decoupled.
