# FH101RFRX – Arduino sketch for the FH101RF (868 MHz)

Arduino sketch that initialises the RFicient® **FH101RF** wake-up receiver
through the **FH101RFBOB** breakout board. It uses the 868 MHz preset
`wurx_conf_868_1k_32k_idm2slow_allbranch_dout1` from the included driver library
(`wurx.c` / `wurx.h`).

The sketch is **complete and ready to use**: the driver sources are in the
folder, no additional libraries have to be installed and no code has to be
added. Just wire it up, select the board, upload.

## Files

| File | Content |
|---|---|
| `FH101RFRX.ino` | Sketch: initialisation, status output, IRQ evaluation |
| `fh101rf_platform.cpp` | Implementation of the driver hooks `wurx_write_reg()`, `wurx_read_reg()`, `wurx_reset_register()` through the Arduino hardware SPI |
| `fh101rf_platform.h` | Includes `wurx.h` with C linkage (required because `.ino` is C++) |
| `fh101rf_config.h` | Pins and SPI clock (adapt here) |
| `wurx.c` / `wurx.h` | Unmodified FH101RF driver library from the repository |

## Configuration (868 MHz)

The preset sets:

- band 868 MHz, all branches (strong, medium, weak)
- preamble (LDR) 1 kbit/s, data (HDR) 32 kbit/s
- LO target 920 MHz
- ID `0x7DA8`, individual ID only (no group/broadcast ID)
- IRQ on ID match + slow event (`WURX_IDMANDSLOW_IRQ`)
- internal 32.768 kHz crystal oscillator
- GPO1 = IRQ signal, GPO2 = static high

`wurx_init()` runs the complete power-up sequence: crystal start-up
(`XTAL_GOOD`), LO calibration and comparator offset calibration. The call takes
typically about 1 s, because `XTAL_GOOD` is only set after roughly 0.5 s.

### 915 MHz (nonEU variant of the BOB)

The FH101RF has three band paths (433, 868, 2G4) — a separate **915 MHz path
does not exist**. 868 and 915 share the same path, the same matching network and
the same SMA connector. What differs between the two BOB variants is only the
**SAW filter in that shared path**:

| BOB variant | Receiving bands | SAW filter in the 868/915 path | Center frequency |
|---|---|---|---|
| `FH101RFBOB_EU_V1.0` | 433 / 868 / 2G4 | RF360 `B39871B3744H110` | **868.3 MHz** |
| `FH101RFBOB_nonEU_V1.0` | 433 / 915 / 2G4 | RF360 `B39921B3300H110` | **916.5 MHz** |

The 433 MHz and the 2.4 GHz positions are identical on both variants (populated
with `B39431B3790Z810` and `B39252B7556P810`); only the 868/915 position is
populated differently.

Consequences:

- **868 and 915 are mutually exclusive on one board.** The EU board cannot
  receive 915 MHz, the nonEU board cannot receive 868 MHz — the SAW filter
  blocks the respective other frequency. A band that is not on the board cannot
  be enabled by software.
- **The preset has to match the fitted filter, not the IC.** Both variants are
  configured through the same 868 MHz band path of the IC, so only the LO target
  changes in software.
- The nonEU filter is centered at **916.5 MHz**, not at 915 MHz. That is why the
  preset is named `916` and why `lo_target_868 = 956` fits so well:
  956 − 916.5 = **39.5 MHz** IF, essentially the datasheet optimum of 40 MHz.
- The sensitivity is the same on both paths: −72 dBm at 868.3 MHz and −72 dBm at
  916.5 MHz.
- The nonEU variant goes with the antenna Linx `ANT-916-CW-HWR-SMA`
  (EU: `ANT-868-CW-HWR-SMA`).

Source: `FH101RFBOB_en_V1R3.pdf`, sections 1 (Introduction), 4 (Board structure,
Table 3), 5 (Operating conditions) and 6 (Ordering information).

### Preset for 915 MHz

For the nonEU variant there is the preset
`wurx_conf_916_1k_32k_idm2slow_allbranch_dout1`. It uses the 868 MHz band path of
the IC and differs from the 868 preset in **exactly one value**:

> **`lo_target_868 = 956` instead of `920`.** Band, branch, FDD and IDM
> selector, data rates, IRQ source, ID and output mux are identical. Keep it
> that way: if the 868 preset is changed, change the 915 preset the same way —
> `lo_target_868` stays the only differing value.

To switch in the sketch, swap the preset variable:

```c
wurx_config_t rxConfig = wurx_conf_916_1k_32k_idm2slow_allbranch_dout1;
```

On an EU board it stays with the 868 preset — the 915 MHz filter is not fitted
there.

## Wiring

| FH101RFBOB | Name | Arduino | Direction | Mandatory |
|---|---|---|---|---|
| 2 | CS_N | `FH101RF_CS_PIN` (default D10) | MCU → BOB | yes |
| 4 | MOSI | MOSI | MCU → BOB | yes |
| 5 | SCLK | SCK | MCU → BOB | yes |
| 3 | MISO | MISO | BOB → MCU | for 4-wire SPI |
| 9 | GPO1 / IRQ | `FH101RF_IRQ_PIN` (default D9) | BOB → MCU | yes |
| 1 | RESET_N | `FH101RF_RESET_PIN` (default not assigned) | MCU → BOB | no |
| 6 | VDD | 3V3 (1.6 V … 3.63 V) | — | yes |
| 7 | GND | GND | — | yes |

Important: the GPIO levels of the BOB are defined by the voltage at VDD. VDD and
the I/O voltage of the MCU therefore have to match.

The BOB supports 3-wire and 4-wire SPI. This sketch uses the Arduino hardware SPI
in 4-wire operation (write MOSI, read MISO), which works with the BOB directly.
The SPI configuration is mode 0, MSB first, 10 MHz maximum.

## Adapting the pins

All hardware parameters are in `fh101rf_config.h` and can also be overridden by
a compiler define:

```cpp
#define FH101RF_CS_PIN      10    // CS_N
#define FH101RF_IRQ_PIN      9    // GPO1
#define FH101RF_RESET_PIN   -1    // RESET_N, -1 = not wired
#define FH101RF_SPI_HZ  4000000UL // max. 10 MHz
```

For the wiring from the repository README (Adafruit Feather M0 RFM69HCW)
`RESET_N` is on D11, so set `FH101RF_RESET_PIN 11` there.

## Build and flash

Arduino IDE: open the folder `FH101RFRX` (the folder name has to match the
`.ino` name), select board and port, upload.

With `arduino-cli`:

```bash
arduino-cli compile --fqbn arduino:avr:uno FH101RFRX
arduino-cli upload  --fqbn arduino:avr:uno -p COM5 FH101RFRX
```

## Serial output (115200 baud)

```
==================================================
 FH101RF wake-up receiver - 868 MHz configuration
==================================================
IC version (reg 0x7F) : 0x41

wurx_init() with 868 MHz preset:
  ...
init time             : 1002 ms
XTAL_GOOD (0x77)      : 1, clock stable
CALIB_STATUS (0x06)   : 0x0
ACTUAL_BANDSELECT     : 0x2 (868 MHz active)
...
receiver state        : OK
receiver is listening for 868 MHz wake-up telegrams
```

At a wake-up event on GPO1 the event record is read and printed (IRQ status, IDM
reason, band, correlator value, counter value, FIFO content).

## Notes

- The receiver needs a stable supply; the BOB draws short current peaks while
  sampling. Do not remove the recommended capacitors of the BOB.
- After power-up `XTAL_GOOD` has to be 1, otherwise `wurx_init()` fails
  (`receiver state: FAILED`). The cause is usually the crystal or the supply.
- The transmitter has to match the preset exactly (OOK, 1 kbit/s preamble,
  32 kbit/s data, ID `0x7DA8`), otherwise no wake-up event is generated.
- `wurx_init()` only returns the overall status. Details (XTAL, LO cal, offset
  cal) are read back in the sketch through the status registers.
- The driver files `wurx.c` and `wurx.h` are included unmodified.

## Plausibility Check

The configuration has been verified against the official FH101RF datasheet and all parameters match the specifications for 868 MHz operation:
- SPI configuration (Mode 0, MSB first, max. 10 MHz) - correct
- Band: 868 MHz with all branches enabled - correct
- Preamble rate: 1 kbit/s - correct
- Data rate: 32 kbit/s - correct
- LO target: 920 MHz - correct
- IRQ source: ID match + slow event - correct
- SAW filter: B39871B3744H110 centered on 868.3 MHz - correct
