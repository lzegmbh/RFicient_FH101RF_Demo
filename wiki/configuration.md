# Configuration

## Presets

The driver ships four presets in `wurx.c` / `wurx.h`:

| Preset | Band | Preamble | Data | LO target | IRQ source |
|---|---|---|---|---|---|
| `wurx_conf_433_1k_32k_idm2slow_allbranch_dout1` | 433 MHz | 1 kbit/s | 32 kbit/s | 470 MHz | `WURX_IDMANDSLOW_IRQ` |
| `wurx_conf_868_1k_32k_idm2slow_allbranch_dout1` | 868 MHz | 1 kbit/s | 32 kbit/s | 920 MHz | `WURX_IDMANDSLOW_IRQ` |
| `wurx_conf_916_1k_32k_idm2slow_allbranch_dout1` | 915 MHz | 1 kbit/s | 32 kbit/s | 956 MHz | `WURX_IDMANDSLOW_IRQ` |
| `wurx_conf_2g4_1k_32k_idm2slow_allbranch_dout1` | 2.4 GHz | 1 kbit/s | 32 kbit/s | 2440 MHz | `WURX_IDMANDSLOW_IRQ` |

All presets use all branches (strong, medium, weak), individual ID `0x7da8`,
`WURX_XTAL_CLK` and `WURX_IRQ_EVENT_HIGH` as output mux.

Select a preset and initialize the receiver:

```c
if(wurx_init(wurx_conf_868_1k_32k_idm2slow_allbranch_dout1))
{
    /* receiverGood: xtal ok, LO cal done, offset cal done */
}
```

`wurx_init()` runs the complete power-up sequence: crystal start-up (`XTAL_GOOD`), LO
calibration and comparator offset calibration. The call takes typically about 1 s, because
`XTAL_GOOD` is only set after roughly 0.5 s.

## 915 MHz

The 915 MHz preset is meant for the **nonEU variant** of the FH101RFBOB (the one with the
433 / 915 / 2G4 SAW filters). The IC itself has no separate 915 MHz band, so the preset drives
the **868 MHz band path** — 868 and 915 share one path on the board and are separated only by
the fitted SAW filter (EU: 868.3 MHz, nonEU: 916.5 MHz). See [Hardware](hardware) for the
filter table.

> **The only difference between the 868 MHz and the 915 MHz setting is the LO target:**
> `lo_target_868 = 956` instead of `920`. Band selector, branch selector, FDD and IDM
> selectors, data rates, IRQ source, ID and output mux are identical. Keep it that way — if the
> 868 preset is ever changed, change the 915 preset the same way and leave `lo_target_868` as
> the single differing value.

To switch in a sketch, swap the preset variable:

```c
wurx_config_t rxConfig = wurx_conf_916_1k_32k_idm2slow_allbranch_dout1;
```

On an EU board it stays with the 868 preset — the 915 MHz filter is not fitted there.

## The LO target is not the carrier frequency

`lo_target_868` is the frequency of the free-running oscillator, **not** the carrier
frequency. An 868.4 MHz carrier lands at |868.4 − fLO| MHz:

| LO target | Carrier | IF | Note |
|---|---|---|---|
| 920 MHz (868 preset) | 868.4 MHz | **52 MHz** | datasheet recommends 30…50 MHz, optimum 40 |
| 908 MHz (experiment) | 868.4 MHz | 40 MHz | measured **worse**, see below |
| 956 MHz (916 preset) | 916.5 MHz | **39.5 MHz** | nonEU filter center, right at the optimum |
| 956 MHz (916 preset) | 915.0 MHz | 41 MHz | a carrier exactly on the band edge |

The 30…50 MHz figure is a recommendation, not a hard limit. The change to 908 MHz was measured
on hardware and made the link **worse**: the first reception needed 10 dBm instead of 0 dBm.
The preset therefore stays at 920 MHz. Details in [Measurements](measurements).

## Sketch switches

The link test `FH101TRX` has all switches at the top of `FH101TRX.ino`:

| Define | Default | Meaning |
|---|---|---|
| `TX_FREQ_HZ` | `868400000UL` | Carrier frequency |
| `TX_REPEAT_MS` | `3000UL` | Transmit interval |
| `TX_ID` | `WURX_TG_DEFAULT_ID` | Wake-up ID, has to match the receiver ID |
| `TX_POWER_DBM` | `13` | Transmit power, −18 … +13 dBm |
| `TX_SEND_ENABLE` | `1` | `1` = transmit, `0` = receive only |
| `RX_LISTEN_MS` | `10000UL` | Length of a receive window (marker in the log only) |

`TX_SEND_ENABLE 0` turns the board into a pure receiver: the RFM69 is initialised and set to its
power, but never keyed. This lets you check an **external** transmitter without your own carrier
sitting on the receiver.

The transmit power is written at runtime through `rfm69_ook_set_power_dbm()`. A `#define` in the
`.ino` is not sufficient on its own, because `RFM69_TX_POWER_DBM` is compiled in as a constant
inside `rfm69_ook.cpp`.

## RFM69 OOK register set

The transmitter driver configures the RFM69HCW like this:

| Register | Address | Value | Meaning |
|---|---|---|---|
| `RegOpMode` | `0x01` | `0x04` / `0x0C` | Standby / TX |
| `RegDataModul` | `0x02` | `0x08` | DataMode `00` = **packet mode**, OOK, no shaping |
| `RegBitrate` | `0x03/0x04` | `0x03C7` = 967 | 32 MHz / 967 = **33092 Hz** |
| `RegFrf` | `0x07..0x09` | `0xD91999` | **868.4 MHz** |
| `RegPreambleMsb/Lsb` | `0x2C/0x2D` | `0x0000` | no additional preamble |
| `RegSyncConfig` | `0x2E` | `0x00` | sync word off |
| `RegPacketConfig1` | `0x37` | `0x00` | fixed length, **no CRC**, no DC-free, no address filtering |
| `RegPacketConfig2` | `0x3D` | `0x00` | no AES, no auto RX restart |
| `RegPayloadLength` | `0x38` | `0x00` | **unlimited length packet format** |
| `RegFifoThresh` | `0x3C` | `0x80` | TX starts as soon as the FIFO is not empty |
| `RegPaLevel` | `0x11` | `0x40 \| (dBm + 18)` | PA_BOOST (PA1), e.g. `0x52` = 0 dBm, `0x5F` = +13 dBm |
| `RegPaRamp` | `0x12` | `0x0F` | shortest ramp (10 µs), no shaping |
| `RegOcp` | `0x13` | `0x1A` | over-current protection on, 95 mA |
| `RegDioMapping2` | `0x26` | `0x07` | clock out off |

**Packet mode is mandatory.** In continuous mode the FIFO and packet handler are inactive
(datasheet 5.4.1): every FIFO write is discarded and the PA transmits a flat, unmodulated
carrier. See [Troubleshooting](troubleshooting) for the full story.
