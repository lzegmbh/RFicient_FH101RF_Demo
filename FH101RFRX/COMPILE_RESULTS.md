# Build results

Reference build sizes for all sketches in this repository.

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

## Configuration under test

- Preset: `wurx_conf_868_1k_32k_idm2slow_allbranch_dout1`
- SAW filter: `B39871B3744H110`, centred on 868.3 MHz
- SPI: mode 0, MSB first, 4 MHz, `CS_N` on pin 10
- IRQ: `GPO1` on pin 9, active high
