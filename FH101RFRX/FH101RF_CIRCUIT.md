# Circuit and wiring

How the FH101RFBOB is connected to a host board, and which limits apply.

## The board is a complete receiver

`FH101RFBOB` is a finished breakout board, not a bare chip. It carries the FH101RF wake-up
receiver, the matching network, the SAW filter and an SMA connector. There is no RF circuit to
build - the only external parts are the supply, the SPI wires and the antenna.

| Item | Value | Source |
|---|---|---|
| Supply voltage on `VDD` (pin 6) | 1.6 V to 3.63 V | BOB datasheet, Table 5 |
| Current into `VDD` | 15 mA | BOB datasheet, Table 5 |
| Digital high input level | min. 2 V, max. `Vvdd` + 0.3 V | BOB datasheet, Table 5 |
| SPI clock | max. 10 MHz, mode 0, MSB first | FH101RF datasheet |
| SAW filter, EU board | `B39871B3744H110`, 868.3 MHz | BOB datasheet, Table 3 |
| SAW filter, nonEU board | `B39921B3300H110`, 916.5 MHz | BOB datasheet, Table 3 |
| Recommended antenna | Linx `ANT-868-CW-HWR-SMA` | BOB datasheet, Table 2 |

## Pin assignment of the BOB

| Pin | Name | Direction | Mandatory | Used in this project |
|---|---|---|---|---|
| 1 | `RESET_N` | in | no, internal pull-up | not wired (`FH101RF_RESET_PIN -1`) |
| 2 | `CS_N` | in | yes | pin 10, low active |
| 3 | `MISO` | out | only for 4-wire SPI | SPI MISO |
| 4 | `MOSI` | in/out | yes | SPI MOSI |
| 5 | `SCLK` | in | yes | SPI clock |
| 6 | `VDD` | supply | yes | 3.3 V |
| 7 | `GND` | ground | yes | GND |
| 8 | `GPO2` | out | register 0x75 | not used |
| 9 | `GPO1` | out | register 0x75 | pin 9, interrupt, active high |

`CS_N` is low active. `RESET_N` is low active too and must **not** be driven high permanently -
the BOB pulls it up itself, so leaving it open is the safest wiring.

## Supply voltage - the one trap

**The BOB must not be run at 5 V.** `VDD` ends at 3.63 V, and the digital inputs must not
exceed `Vvdd` + 0.3 V. The datasheet states the consequence explicitly: the GPIO levels of the
board follow the supply on `VDD`, so a board supplied with 3 V also expects 3 V on `CS_N`,
`MOSI` and `SCLK`.

This decides the host board:

| Host | Supply | Logic level | Wiring |
|---|---|---|---|
| Adafruit Feather M0 RFM69HCW | 3.3 V | 3.3 V | direct, no extra parts |
| Arduino Uno / Nano | 3.3 V from the `3V3` pin | 5 V | level shifter required on `CS_N`, `MOSI`, `SCLK` |

On an Uno the SPI lines idle at 5 V, which is above the allowed maximum. Feeding them directly
into a 3.3 V supplied BOB overstresses the input protection. Use a level shifter (for example a
74LVC245 or a resistor divider on `CS_N` and `SCLK`), or use a 3.3 V board and avoid the problem
entirely - which is why the Feather M0 is the primary target of this project.

The `3V3` pin of an Uno delivers enough current for the 15 mA the BOB draws, but keep the wires
short: the receiver pulls short current peaks, and the BOB is delivered with its own decoupling
capacitors. Do not remove them, and do not add long jumper wires between the capacitors and the
board.

## Wiring to the Adafruit Feather M0 RFM69HCW

| BOB pin | Feather pin | Note |
|---|---|---|
| 6 `VDD` | `3V3` | 3.3 V, never `USB` or `5V` |
| 7 `GND` | `GND` | common ground |
| 5 `SCLK` | `D24` | SPI clock |
| 4 `MOSI` | `D23` | SPI MOSI |
| 3 `MISO` | `D22` | SPI MISO |
| 2 `CS_N` | `D10` | chip select, low active |
| 9 `GPO1` | `D9` | interrupt, active high |

`D3`, `D4` and `D8` belong to the on-board RFM69HCW and must stay free. If you want to wire
`RESET_N` as well, set `FH101RF_RESET_PIN` to the pin you chose in `fh101rf_config.h`.

## Wiring to an Arduino Uno

| BOB pin | Uno pin | Note |
|---|---|---|
| 6 `VDD` | `3V3` | **not** `5V` |
| 7 `GND` | `GND` | |
| 5 `SCLK` | `D13` | through level shifter |
| 4 `MOSI` | `D11` | through level shifter |
| 3 `MISO` | `D12` | direct, the BOB drives 3.3 V |
| 2 `CS_N` | `D10` | through level shifter |
| 9 `GPO1` | `D9` | direct, 3.3 V is a valid high level for the Uno |

`MISO` and `GPO1` go from the BOB to the Uno and need no shifter: 3.3 V exceeds the 2 V minimum
high level of an AVR. Only the three inputs of the BOB need shifting.

## Antenna

The board has one SMA connector per band path. Screw on a 50 ohm antenna for the band you use -
868 MHz for the EU board, 915 MHz for the nonEU board. The 433 MHz and 2.4 GHz paths are
separate connectors and are not affected by this choice.

Without an antenna the receiver still works, but only at very short range. The same applies to
the transmitter side: an RFM69 without an antenna radiates a few centimetres only, which is why
the link test in this repository runs at +13 dBm. See the antenna note in the top level
`README.md`.

## Band selection is a hardware property

The IC has three signal paths - 433 MHz, 868/915 MHz and 2.4 GHz. There is no separate 915 MHz
path: the 868 and 915 MHz bands share the same path, matching network and SMA connector, and
only the SAW filter differs between the two board versions. A board therefore supports either
868 MHz or 915 MHz, never both, and no register setting can change that.

## Checklist when the receiver does not answer

1. `VDD` between 1.6 V and 3.6 V, measured at the BOB.
2. Common ground between host and BOB.
3. `CS_N` on the pin `FH101RF_CS_PIN` names.
4. SPI mode 0 and MSB first - the FH101RF is not configurable here.
5. `RESET_N` not held high.
6. No other device active on the bus while the receiver is addressed.

The driver reports what it found on `RegVersion`; the meaning of each value is in the top level
`README.md` under *Troubleshooting*.
