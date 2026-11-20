# Getting started

## What you need

| Part | Note |
|---|---|
| Adafruit Feather M0 RFM69HCW | SAMD21 board with the RFM69HCW transceiver on board |
| FH101RFBOB | Breakout board of the FH101RF wake-up receiver, EU or nonEU variant |
| 9 jumper wires | The BOB pin header is 0.1 in, the Feather has matching headers |
| USB cable | Power, programming and the serial console |
| Antenna for the BOB | SMA antenna matching the fitted SAW filter (see [Hardware](hardware)) |

An antenna for the **RFM69** is not included with the Feather. The board works without one,
but only over a few centimetres — see the warning in [Hardware](hardware).

## Step 1 — wire it up

The mandatory signals are VDD, GND, SCLK, MOSI, CS_N and GPO1. The complete table is in
[Wiring](wiring); the short version for the Feather M0 RFM69HCW is:

| FH101RFBOB pin | Feather pin |
|---|---|
| 6 VDD | 3V3 |
| 7 GND | GND |
| 2 CS_N | D10 |
| 5 SCLK | D24 (SCK) |
| 4 MOSI | D23 |
| 3 MISO | D22 |
| 9 GPO1 | D9 |

Do **not** use D3, D4 or D8 — they belong to the on-board RFM69HCW.

## Step 2 — build and upload

The sketches are complete: every driver source they need is inside the sketch folder, there is
no library to install and no code to fill in. Open the folder in the Arduino IDE (the folder
name has to match the `.ino` name) or use `arduino-cli`:

```bash
arduino-cli compile --fqbn adafruit:samd:adafruit_feather_m0 FH101TRX
arduino-cli upload  --fqbn adafruit:samd:adafruit_feather_m0 -p COM10 FH101TRX
```

`arduino-cli upload` does not accept `--build-property`; switches have to be set in the sketch.

## Step 3 — watch the serial console

Open the port at **115200 baud**. A healthy start looks like this:

```
==================================================
 FH101TRX - FH101RF wake-up transceiver test
==================================================
tx: RFM69HCW, 868399963 Hz, 33092 chip/s, PA_BOOST
tx: power 13 dBm, RegPaLevel 0x5F (verified)
tx: ID 0x7DA8, payload AF FE, every 3000 ms
tx: mode sending
rx: IC version 0x41
rx: wurx_init() with the 868 MHz preset
    (xtal, LO and offset calibration)
rx: LO target 920 MHz -> IF 52 MHz
rx: init 111 ms, XTAL_GOOD 0x1, CALIB_STATUS 0x0, OK
rx: ACTUAL_BANDSELECT 0x2 (868 MHz active)
rx: BAND_BRANCH_CTRL 0x27, IRQ_SELECT 0x20, ID 0x7DA8

ready: sending every 3000 ms, listening for wake-up events

tx #1: ID 0x7DA8, 2 payload byte(s), 260 byte(s), 62855 us, 13 dBm
rx #1 after 1614 ms: IRQ_STATUS 0x20, ID individual, band 868, counter 51449358, payload AF FE 00 00 00 00  -> payload OK
   link: 1 of 1 telegrams received
```

The lines that matter:

| Line | Meaning |
|---|---|
| `IC version 0x41` | The BOB answers on SPI |
| `XTAL_GOOD 0x1` | The 32.768 kHz crystal is running |
| `CALIB_STATUS 0x0` | LO and comparator offset calibration succeeded |
| `ACTUAL_BANDSELECT 0x2` | The 868 MHz band path is active |
| `payload OK` | The received payload matches what was sent |
| `link: N of M` | Link statistic, `N == M` is the goal |

If one of these is missing, go to [Troubleshooting](troubleshooting).

## Step 4 — judge the range honestly

The on-board RFM69HCW has **no antenna**, so the default sketch transmits at +13 dBm just to
bridge a few centimetres. Before you draw any conclusion about the receiver sensitivity, fit an
antenna and set `TX_POWER_DBM` back to a realistic −10 dBm. Details and the measured power
sweep are in [Measurements](measurements).

## Next steps

- Switch to a different band: [Configuration](configuration)
- Understand the telegram the transmitter produces: [Signal format](signal-format)
- Build your own firmware on the driver: [Driver API](driver-api)
