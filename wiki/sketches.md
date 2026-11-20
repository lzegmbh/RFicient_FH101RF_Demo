# Sketches

All example projects in this repository are **complete, ready-to-use Arduino sketches**. They
compile as they are, contain every driver source they need and work with nothing but the wiring
shown in [Wiring](wiring) — no extra libraries to install and no code to fill in.

| Sketch | Board | What it does | Size |
|---|---|---|---|
| `FH101TRX` | Feather M0 RFM69HCW + FH101RFBOB | Complete link test on a single board: the on-board RFM69HCW sends, the FH101RFBOB receives. **Start here.** | 28664 B |
| `FH101RFRX` | Feather M0 RFM69HCW + FH101RFBOB | Receiver only: configures the FH101RF, prints the init status and reports every wake-up event | 25708 B |
| `WurxTelegramSendRfm69` | Feather M0 RFM69HCW | Transmitter only: sends the wake-up telegram with the on-board RFM69HCW | 22288 B |
| `WurxTelegramDump` | any Arduino board | No radio and no FH101RF needed: generates the telegram, decodes it back and prints it as a hexdump | 4170 B |

Every sketch folder carries its own copy of the driver sources it uses, so the folders are
self-contained and can be copied out of the repository on their own. The only thing to adapt is
the pin assignment in `fh101rf_config.h` if the wiring differs.

## FH101TRX — the link test

The shortest way to see the whole chain working. One board sends and receives at the same time:
the RFM69HCW transmits a wake-up telegram every `TX_REPEAT_MS`, the FH101RFBOB listens for it.

- TX: `rfm69_ook.cpp`, OOK packet mode, telegram from `wurx_telegram.c`
- RX: `wurx.c` with the 868 MHz preset
- Output: IRQ status, ID type, band, counter, payload and a `link: N of M` statistic

Both chips share one SPI bus, so the three rules from [Wiring](wiring) apply. This sketch is the
reference implementation for that.

## FH101RFRX — receiver only

Initialises the FH101RF, prints the complete init status and then reports every wake-up event.
Use it when the transmitter is a separate device, or when you want to check the receiver side in
isolation.

Output of a healthy start:

```
IC version (reg 0x7F) : 0x41
init time             : 1002 ms
XTAL_GOOD (0x77)      : 1, clock stable
CALIB_STATUS (0x06)   : 0x0
ACTUAL_BANDSELECT     : 0x2 (868 MHz active)
receiver state        : OK
receiver is listening for 868 MHz wake-up telegrams
```

## WurxTelegramSendRfm69 — transmitter only

Sends the same telegram with the on-board RFM69HCW (SX1231H). The chip runs in **OOK packet mode
with unlimited length packet format**: the bytes are written into the 66 byte FIFO and the chip
clocks them out bit by bit at the rate set in `RegBitrate`. The chip stream is therefore as
accurate as the module crystal — no bit-banging in the main program.

Serial console commands:

| Key | Function |
|---|---|
| `s` | send one telegram |
| `t` / `T` | telegram repetition on / off |

Without a key press the sketch sends every `TX_REPEAT_MS` (default 2000 ms).

## WurxTelegramDump — no hardware needed

Generates an ID-only and an ID+payload telegram, prints size, chip count, duration and hexdump
and **decodes the result back** (preamble, ID, payload), so the serial output proves the
correctness directly:

```
--- ID only ---
ID                 : 0x7DA8, payload 0 byte(s)
telegram size      : 196 byte(s)
chips              : 1568, duration at 33096 Hz: 47 ms
preamble           : 32 bit at 1024 bit/s, spread x32
verify             : OK
```

This is the sketch to use when you only want to verify the telegram generator, or when you need
the chip stream for a different transmitter.

## Building

```bash
arduino-cli compile --fqbn adafruit:samd:adafruit_feather_m0 FH101TRX
arduino-cli compile --fqbn adafruit:samd:adafruit_feather_m0 FH101RFRX

arduino-cli compile --fqbn arduino:avr:uno --library FH101RFTX FH101RFTX/examples/WurxTelegramDump
arduino-cli compile --fqbn adafruit:samd:adafruit_feather_m0 --library FH101RFTX FH101RFTX/examples/WurxTelegramSendRfm69
```

The two examples inside `FH101RFTX/` use `--library FH101RFTX` because they include
`wurx_telegram.h` from the library folder. In the Arduino IDE the library has to be installed
(copy or link the folder `FH101RFTX` into `<sketchbook>/libraries/`).

Note: `examples/WurxTelegramDump/` additionally contains copies of `wurx_telegram.c` and
`wurx_telegram.h`. These copies take precedence over the library as long as they sit in the
sketch folder.
