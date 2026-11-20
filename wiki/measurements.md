# Measurements

All numbers on this page were measured on real hardware: Adafruit Feather M0 RFM69HCW with an
FH101RFBOB (EU variant, 868 MHz SAW filter) wired as shown in [Wiring](wiring).

## Link test

```
link: 14 of 14 telegrams received
rx #N: IRQ_STATUS 0x20, ID individual, band 868, payload AF FE 00 00 00 00  -> payload OK
```

One event every 3062 ms, no dropouts over the whole run. A second run after the sketch was
cleaned up gave 9 of 9.

Receiver init, all green:

| Item | Value |
|---|---|
| IC version | `0x41` |
| `XTAL_GOOD` | `0x1` |
| `CALIB_STATUS` | `0x0` |
| `ACTUAL_BANDSELECT` | `0x2` (868 MHz active) |
| `BAND_BRANCH_CTRL` | `0x27` |
| `IRQ_SELECT` | `0x20` |
| Init time | ~111 ms (the driver allows up to ~1 s) |
| Telegram duration | 62855 µs |
| Counter increment per telegram | +100389 (32.768 kHz ticks of the receiver, not telegrams) |

## Transmit power sweep

The Feather has no antenna on the RFM69. A sweep with one power step per telegram shows what
that costs:

| Power | `RegPaLevel` | Reception |
|---|---|---|
| −18 dBm | `0x40` | ✗ |
| −10 dBm | `0x48` | ✗ |
| −4 dBm | `0x4E` | ✗ |
| 0 dBm | `0x52` | ✓ |
| 5 dBm | `0x57` | ✓ |
| 10 dBm | `0x5C` | ✓ |
| 13 dBm | `0x5F` | ✓ |

The threshold sits at 0 dBm, roughly **18 dB above** what a matched antenna would need. That is
the missing antenna, not a receiver problem.

### The receiver was verified separately

The same setup was used to listen to an **external** transmitter at 15 cm distance with the
on-board RFM69 switched off (`TX_SEND_ENABLE 0`). Result: **17 of 17** telegrams received at
full sensitivity. The receive chain was never the fault.

Conclusion: keep `TX_POWER_DBM 13` while the board has no antenna, and set it back to a
realistic −10 dBm once an antenna is fitted.

## Intermediate frequency experiment

`lo_target_868` is the free-running oscillator, not the carrier. The 868 preset uses 920 MHz,
which puts an 868.4 MHz carrier at 52 MHz IF — outside the 30…50 MHz the datasheet recommends
(optimum 40 MHz).

The obvious fix was tried: `RX_LO_TARGET_MHZ 908` gives exactly 40 MHz IF.

| LO target | IF | First reception needed |
|---|---|---|
| 920 MHz (preset) | 52 MHz | **0 dBm** |
| 908 MHz (experiment) | 40 MHz | 10 dBm |

The "optimum" was **worse** by 10 dB. The 30…50 MHz window is a recommendation, not a limit;
the correlator copes with the wider IF. The preset was left at 920 MHz.

This is also why the 915 MHz preset at 956 MHz is a good fit: 956 − 916.5 = 39.5 MHz, right at
the datasheet optimum, without touching the 868 setting.

## Compile sizes

| Sketch | FQBN | Flash |
|---|---|---|
| `FH101TRX` | `adafruit:samd:adafruit_feather_m0` | 28664 B (10 %) |
| `FH101RFRX` | `adafruit:samd:adafruit_feather_m0` | 25708 B |
| `WurxTelegramSendRfm69` | `adafruit:samd:adafruit_feather_m0` | 22288 B |
| `WurxTelegramDump` | `arduino:avr:uno` | 4170 B |

## Telegram generator verification

Checked against the examples from the datasheet:

| ID | Expected sequence (fast) | Result |
|---|---|---|
| `0xB000` | `BABB AAAA AAAA AAAA` | OK |
| `0x032F` (broadcast) | `AAAA AABB AABA BBBB` | OK |

Plus a round-trip test covering preamble spreading, pause, ID and payload decoding, lengths and
error cases.
