# Build results

Reference build sizes for all sketches in this repository, and the analysis of one linker
error that this project produced.

All numbers were measured with `arduino-cli` and are reproducible with the commands below.

## Verified build sizes

| Sketch | Board | Flash | RAM |
|---|---|---|---|
| `FH101RFRX` | Arduino Uno (`arduino:avr:uno`) | 6090 B (18 %) | 204 B (9 %) |
| `FH101RFRX` | Feather M0 (`adafruit:samd:adafruit_feather_m0`) | 25708 B (9 %) | - |
| `FH101TRX` | Feather M0 | 28664 B (10 %) | - |
| `WurxTelegramSendRfm69` | Feather M0 | 22288 B (8 %) | - |
| `WurxTelegramDump` | Arduino Uno | 4170 B (12 %) | - |

```bash
arduino-cli compile --fqbn arduino:avr:uno FH101RFRX
arduino-cli compile --fqbn adafruit:samd:adafruit_feather_m0 FH101RFRX
arduino-cli compile --fqbn adafruit:samd:adafruit_feather_m0 FH101TRX
arduino-cli compile --fqbn adafruit:samd:adafruit_feather_m0 --library FH101RFTX FH101RFTX/examples/WurxTelegramSendRfm69
arduino-cli compile --fqbn arduino:avr:uno --library FH101RFTX FH101RFTX/examples/WurxTelegramDump
```

## Case study: `multiple definition of wurx_write_reg`

### Symptom

`FH101RFRX` failed to link with eleven duplicate symbols:

```
(.text+0x0): multiple definition of `wurx_write_reg'
(.text+0x0): multiple definition of `wurx_read_reg'
(.text+0x0): multiple definition of `wurx_reset_register'
(.text+0x0): multiple definition of `wurx_irq_handler'
(.text+0x0): multiple definition of `wurx_init'
(.text+0x0): multiple definition of `wurx_conf_868_1k_32k_idm2slow_allbranch_dout1'
(.text+0x0): multiple definition of `delay'
(.text+0x0): multiple definition of `pinMode'
(.text+0x0): multiple definition of `digitalWrite'
(.text+0x0): multiple definition of `digitalRead'
collect2.exe: error: ld returned 1 exit status
```

### Wrong conclusion

The obvious reading is "the `__attribute__((weak))` declarations in `wurx.h` clash with the
definitions in `wurx.c`", and the obvious fix is to delete those declarations from the header.

That is wrong, and it is harmful: the weak declarations are what allows a sketch to use the
presets without implementing the SPI hooks. Removing them breaks every standalone example.

### Actual cause

The sketch folder contained a second source file, `compile_test.c` - a host-side scratch file
that stubs the Arduino core and the driver so it can be compiled with a desktop compiler. The
Arduino build compiles **every** `.c` and `.cpp` file in a sketch folder, so that file was
linked into the firmware alongside `wurx.c`.

The list of duplicate symbols proves it: `delay`, `pinMode`, `digitalWrite` and `digitalRead`
appear next to the driver symbols. Those four come from the Arduino core, and only a file that
stubs them can collide with the core.

### Fix

Keep only the six sketch files in the folder:

```bash
# move any scratch file out of the sketch folder
mv FH101RFRX/compile_test.c ../scratch/
arduino-cli compile --fqbn arduino:avr:uno FH101RFRX
```

Result: builds clean, 6090 B. Nothing in `wurx.h` or `wurx.c` has to be touched.

### Rule

Never place a `.c`, `.cpp` or `.h` file in a sketch folder that is not part of the sketch.
`wurx.h` and `wurx.c` are the single exception, because the driver is intentionally compiled
in place. Scratch code belongs outside the sketch folder.

## Configuration under test

- Preset: `wurx_conf_868_1k_32k_idm2slow_allbranch_dout1`
- SAW filter: `B39871B3744H110`, centred on 868.3 MHz
- SPI: mode 0, MSB first, 4 MHz, `CS_N` on pin 10
- IRQ: `GPO1` on pin 9, active high
