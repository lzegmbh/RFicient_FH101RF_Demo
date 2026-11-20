# Wiring

The FH101RFBOB pin header is connected to the Feather. The GPIO levels of the BOB are defined
by its VDD pin, so VDD and the MCU I/O voltage must match.

## Connection diagram

```
   Adafruit Feather M0 RFM69HCW              FH101RFBOB (pin header)
   +---------------------------+            +------------------------+
   |                           |            |                        |
   |  3V3  --------------------+------------+-- 6  VDD   1.6-3.63 V |
   |  GND  --------------------+------------+-- 7  GND               |
   |                           |            |                        |
   |  SCK  (D24) --------------+------------+-- 5  SCLK   max 10 MHz |
   |  MOSI (D23) --------------+------------+-- 4  MOSI              |
   |  MISO (D22) --------------+------------+-- 3  MISO              |
   |  D10  --------------------+------------+-- 2  CS_N              |
   |                           |            |                        |
   |  D9   --------------------+------------+-- 9  GPO1 / IRQ        |
   |  D11  --------------------+------------+-- 1  RESET_N  optional |
   |  D6   --------------------+------------+-- 8  GPO2 / IRQ  opt.  |
   |                           |            |                        |
   +---------------------------+            +------------------------+

   mandatory : VDD, GND, SCLK, MOSI, CS_N, GPO1
   optional  : RESET_N (internal pull-up), MISO (3-wire SPI), GPO2
   reserved  : D3, D4, D8 are used by the on-board RFM69HCW
```

## Pin assignment

| FH101RFBOB pin | Name | Feather M0 | Direction | Note |
|---|---|---|---|---|
| 1 | RESET_N | D11 | MCU → BOB | optional, BOB has an internal pull-up |
| 2 | CS_N | D10 | MCU → BOB | mandatory, SPI slave select |
| 3 | MISO | MISO (D22) | BOB → MCU | 3-wire SPI: shared with MOSI |
| 4 | MOSI | MOSI (D23) | MCU → BOB | mandatory |
| 5 | SCLK | SCK (D24) | MCU → BOB | mandatory, max. 10 MHz |
| 6 | VDD | 3V3 | — | 1.6 V … 3.63 V |
| 7 | GND | GND | — | mandatory |
| 8 | GPO2 | D6 | BOB → MCU | optional, interrupt output, function set by register 0x75 |
| 9 | GPO1 | D9 | BOB → MCU | interrupt output, function set by register 0x75 |

The CS and IRQ pins are configurable in the platform layer (`fh101rf_config.h`); D10 and D9 are
the proposed assignment. Use a pin that supports deep-sleep wake-up if the MCU is meant to be
woken by the receiver.

SPI configuration: **mode 0, MSB first, 4 MHz** in the sketches (the BOB allows up to 10 MHz).
The BOB supports 3-wire and 4-wire SPI; the sketches use 4-wire.

## Both radio chips on one bus

In the single-board link test (`FH101TRX`) both radio chips share the same SPI bus:

| Signal | RFM69HCW (on-board) | FH101RFBOB |
|---|---|---|
| CS | D8 | D10 (`CS_N`) |
| IRQ / GPO | D3 (DIO0) | D9 (`GPO1`) |
| RESET | D4 (active HIGH) | not wired |
| SCK | D24 | D24 |
| MOSI | D23 | D23 |
| MISO | D22 | D22 |

### Rules for the shared bus

These three points are not a matter of style, they are the precondition for the setup to work
at all:

1. **Exactly one chip may be selected at a time.** `spiBusIdle()` pulls both CS HIGH and wraps
   every radio access in the sketch.
2. **The direction bits are opposite:** on the RFM69 bit 7 = 1 means *write*, on the FH101RF
   bit 7 = 1 means *read*. An accidental access to the wrong chip is therefore a **valid
   command** there and is executed instead of ignored.
3. **`rfm69_ook_send()` keeps one SPI transaction open for the whole telegram (~63 ms).** The
   FH101RF must not be touched inside that window. The sketch therefore reads the latched IRQ
   **before** the next transmission.

In addition the CS of the FH101RF has to be firmly HIGH before the first SPI traffic: `setup()`
sets both CS before any driver initialises the bus. The FH101RF has no wired reset (`RESET_N`
open) and cannot recover from a foreign register access by itself.

## Adapting the pins

All hardware parameters are in `fh101rf_config.h` and can also be overridden by a compiler
define:

```cpp
#define FH101RF_CS_PIN      10    // CS_N
#define FH101RF_IRQ_PIN      9    // GPO1
#define FH101RF_RESET_PIN   -1    // RESET_N, -1 = not wired
#define FH101RF_SPI_HZ  4000000UL // max. 10 MHz
```

For the wiring shown above `RESET_N` is on D11, so set `FH101RF_RESET_PIN 11` if you want to
use it.
