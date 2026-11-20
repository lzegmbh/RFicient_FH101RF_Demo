# Signal format

The transmitter has to match the receiver configuration, otherwise no wake-up event is
generated. This page describes what is on the air.

## Telegram structure

According to datasheet section 5.6 (figure 6), table 16 and table 23:

| Section | Transmit rate | Length | Content |
|---|---|---|---|
| 1. Preamble | 1 kbit/s (LDR) | 32 bit = 32 ms | CodeA `0x6ACD674F`, every bit spread over 32 chips |
| 2. Pause | — | 32 chips = 0.98 ms | Transmitter pause, receiver switches to HDR (TP1 = 5.371 ms max.) |
| 3. ID block | 32 kbit/s (HDR) | 16 bit = 15.6 ms | ID MSB first, `CodeA` = 0, `CodeB` = 1 |
| 4. Payload | 32 kbit/s (HDR) | `size` × 8 bit | User data MSB first, same encoding |

- **CodeA** (logical 0): `0x6A 0xCD 0x67 0x4F` (mlsA, datasheet table 23)
- **CodeB** (logical 1): `0x6D 0x38 0x97 0x73` (mlsB, datasheet table 23)

The preamble is transmitted at 1 kbit/s, the data at 32 kbit/s. Because the chip stream runs
continuously at 32.768 kchip/s, every preamble bit is repeated 32 times (spreading factor
`WURX_TG_PREAMBLE_SPREAD = 32`).

The generated buffer is bit-packed: bit 7 of byte 0 is the first chip. Chip = 1 means RF on,
chip = 0 means RF off.

## Chip rate

The chips have to be clocked at `WURX_TG_TX_CHIP_RATE_HZ` = **33096 Hz**. According to datasheet
section 5.8 that is 1 % more than the sampling rate of the receiver (32768 Hz), i.e.
`fTX = 1.01 × 32768`.

The RFM69HCW derives this from its own crystal: `RegBitrate = 967` gives 32 MHz / 967 =
33092 Hz, which is 1.0099 × 32768 Hz — inside tolerance.

## Timing

| Telegram | Chips | Duration at 33096 Hz |
|---|---|---|
| ID only | 1568 | 47.4 ms |
| ID + 1 byte | 1824 | 55.1 ms |
| ID + 2 bytes | 2080 | 62.9 ms |
| ID + 6 bytes | 3104 | 93.8 ms |

After the telegram the receiver has to fall back to LDR (TP2 = 1.53 ms) before the next
telegram starts.

The default link test uses ID `0x7DA8` with a 2 byte payload `AF FE`, which is 260 bytes /
2080 chips / 62855 µs on the air.

## Buffer size

`wurx_tg_generate()` uses an internal buffer of size `WURX_TG_BUFFER_SIZE` (default 388 bytes =
ID + 6 bytes payload). For longer payloads increase `WURX_TG_BUFFER_SIZE` or use
`wurx_tg_build()` with its own buffer. Space required: `196 + 32 × size` bytes.

The receiver FIFO holds only 16 bit = 2 bytes by default (register 0x56 `FIFO_LENGTH`, values
16/24/32/40 bit). Longer payloads can be transmitted, but the receiver only captures them up to
the configured FIFO length.

## OOK requirements

- The transmitter has to deliver **true OOK modulation**: below −60 dBm (better −80 dBm) in the
  low level. A carrier that only dips slightly is not detected.
- The data rate has to stay within tolerance, otherwise the receiver detects no chips.
- MSB first. The library already generates the sequence in that order.

## Next

- [Configuration](configuration) — presets, bands and the LO target
- [Driver API](driver-api) — how the receiver side of the format is configured
