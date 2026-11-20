# Building the FH101RF receiver example

How to compile `FH101RFRX` from the command line and from the Arduino IDE.

## Prerequisites

| Tool | Version | Note |
|---|---|---|
| Arduino CLI | 0.32 or newer | or the Arduino IDE 1.8.x / 2.x |
| AVR core | `arduino:avr` | for Arduino Uno / Nano |
| SAMD core | `adafruit:samd` | for the Adafruit Feather M0 |

```bash
arduino-cli core update-index
arduino-cli core install arduino:avr
arduino-cli core install adafruit:samd
```

## What has to be in the sketch folder

The Arduino build compiles **every** `.c`, `.cpp` and `.ino` file it finds in the sketch
folder. The folder must therefore contain exactly these six files and nothing else:

```
FH101RFRX/
├── FH101RFRX.ino           sketch: configuration and main loop
├── fh101rf_config.h        pin assignment and SPI clock
├── fh101rf_platform.cpp    SPI and GPIO implementation of the driver hooks
├── fh101rf_platform.h      prototypes of those hooks
├── wurx.c                  FH101RF driver
└── wurx.h                  FH101RF driver header
```

Any additional source file in that folder is compiled and linked too. A stray test file that
defines `wurx_write_reg()`, `wurx_read_reg()`, `delay()` or `pinMode()` collides with the real
driver and with the core - see [COMPILE_RESULTS.md](COMPILE_RESULTS.md).

## Compiling with the Arduino CLI

```bash
# Arduino Uno / Nano
arduino-cli compile --fqbn arduino:avr:uno FH101RFRX

# Adafruit Feather M0 RFM69HCW
arduino-cli compile --fqbn adafruit:samd:adafruit_feather_m0 FH101RFRX
```

Both configurations compile without an error. Do not pass `--library`: the sketch folder is
self-contained and the driver files are compiled in place.

## Uploading

```bash
arduino-cli upload --fqbn arduino:avr:uno -p COM5 FH101RFRX
arduino-cli upload --fqbn adafruit:samd:adafruit_feather_m0 -p COM10 FH101RFRX
```

`arduino-cli upload` does not accept `--build-property`. Put any additional define into the
sketch or into `fh101rf_config.h` instead.

## Compiling with the Arduino IDE

1. Open `FH101RFRX/FH101RFRX.ino`.
2. Select the board under **Tools > Board** - *Arduino Uno* or *Adafruit Feather M0*.
3. **Sketch > Verify**.

The IDE handles the sketch folder the same way the CLI does, so the "one folder, six files"
rule applies here too.

## Checking the result

A successful build prints the size of the image:

```
Sketch uses 6090 bytes (18%) of program storage space.   # Arduino Uno
Sketch uses 25708 bytes (9%) of program storage space.   # Feather M0
```

If the linker reports `multiple definition of ...` instead, a second source file has crept
into the sketch folder. The reference numbers for all four sketches are in
[COMPILE_RESULTS.md](COMPILE_RESULTS.md).

## Hardware settings

The build does not configure the hardware - `fh101rf_config.h` does:

| Define | Default | Meaning |
|---|---|---|
| `FH101RF_CS_PIN` | 10 | SPI chip select, `CS_N` on the BOB, low active |
| `FH101RF_IRQ_PIN` | 9 | interrupt input from `GPO1`, active high with the 868 preset |
| `FH101RF_RESET_PIN` | -1 | `RESET_N` not wired; the BOB has its own pull-up |
| `FH101RF_SPI_HZ` | 4000000 | 4 MHz; the FH101RF allows up to 10 MHz |

Every value can be overridden from outside with `-DFH101RF_CS_PIN=5` without editing the file.
SPI is always mode 0, MSB first.

**Supply voltage.** The BOB accepts 1.6 V to 3.6 V on `VDD`, and its digital inputs must not
exceed that voltage. A 5 V board therefore needs level shifting. Details in
[FH101RF_CIRCUIT.md](FH101RF_CIRCUIT.md).
