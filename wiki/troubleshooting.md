# Troubleshooting

Every entry here comes from a fault that actually occurred during bring-up of this project.

## Nothing is received at all

Check in this order:

1. **Antenna.** The on-board RFM69HCW ships without one and only reaches a few centimetres. The
   measured threshold was 0 dBm with no antenna — see [Measurements](measurements). If
   `TX_POWER_DBM` is at a realistic value such as −10 dBm, the link cannot work. Either raise the
   power to 13 dBm for the test or fit an antenna.
2. **Init status.** `XTAL_GOOD` has to be `0x1`, `CALIB_STATUS` `0x0`. If not, the crystal or the
   supply is the problem — the BOB draws short current peaks, do not remove its capacitors.
3. **Preset vs. board.** An EU board cannot hear 915 MHz and a nonEU board cannot hear 868 MHz.
   See [Hardware](hardware).
4. **Payload check.** A payload mismatch still produces an event, so a missing event is never a
   payload problem. See the FIFO entry below.

## `RFM69HCW not found`

The sketch prints the version it actually read, the SPI clock and the result of a register
write/read test:

| Output | Meaning |
|---|---|
| `RegVersion : 0x24` | Module detected, everything fine |
| `RegVersion : 0x00` + `register test FAILED` | SPI bus delivers no data — wrong board, wrong pins or no supply |
| `RegVersion : 0x00` + `register test OK` | Bus works, but the part is not an RFM69 (or is held in reset) |
| `RegVersion : 0x12` | it is an SX127x/LoRa module, not an RFM69 |

The original cause of this error was a missing `SPI.begin()`. On SAMD,
`SPI.beginTransaction()` alone does not enable the SERCOM peripheral — every transfer reads
`0x00`. `rfm69_ook_begin()` now calls `SPI.begin()` itself and retries `RegVersion` with a
descending clock (8 MHz, 4 MHz, 1 MHz, 250 kHz).

## `RegVersion` reads fine but every write is ignored

The RESET pin of the RFM69 (D4) is **active HIGH**. While D4 is HIGH the chip still answers
reads (returning the reset defaults) but silently discards every write. Symptoms:

- `RegVersion` reads `0x24`, the module counts as detected
- a register write/read test fails (`0x00` instead of `0xA5`)
- a register dump shows only defaults (`RegBitrate = 0x1A0B`, `RegFrf = 0x000000`,
  `RegPaLevel = 0x9F`)
- the transmit duration is far too short, because the bit rate was never programmed

Proven with bit-banged SPI on the same board:

| D4 | Write access |
|---|---|
| LOW | works |
| HIGH | is ignored (reading still works) |
| open (input) | works, the module pulls the pin down itself |

`rfm69Reset()` pulses D4 briefly HIGH and leaves it **LOW** afterwards.

## The PA transmits, but nothing is modulated

This was the hardest fault. The spectrum analyser showed a signal, but with 4.4 % envelope swing
instead of ~90 %.

Cause: the RFM69 was configured in **continuous mode** (`RegDataModul = 0x48`). Datasheet
SX1231H 5.4.1: *"in Continuous mode the NRZ data to (from) the (de)modulator is directly
accessed by the uC on the bidirectional DIO2/DATA pin. The FIFO and packet handler are thus
inactive."* Every FIFO write was discarded and the PA sent a flat carrier.

The FIFO round-trip test made it obvious:

```
fifo @ datamodul 0x0  : 8/8, first 0x11     <- packet mode, FIFO works
fifo @ datamodul 0x48 : 0/8, first 0x0      <- continuous, FIFO switched off
```

Fix: **packet mode with unlimited length packet format** — `RegDataModul = 0x08`,
`PacketFormat = 0`, `PayloadLength = 0x00`, `PreambleLength = 0`, `SyncOn = 0`. The packet
handler then appends nothing and the FIFO bytes go into the modulator 1:1. Since then the drain
time scales with `1/RegBitrate` (46768 µs at 967, 93467 µs at 1934, 225740 µs at 4835,
23478 µs at 484) and the envelope shows 93 % swing.

## Payload compare never matches

The receiver FIFO returns **its whole depth**, not just the received bytes. A telegram with two
payload bytes appears as `AF FE 00 00 00 00`, so a test for `fifo_count == 2` can never pass.

Compare only the leading `TX_PAYLOAD_SIZE` bytes. The default FIFO length is 16 bit = 2 bytes
(register 0x56 `FIFO_LENGTH`, values 16/24/32/40 bit); longer payloads are silently truncated.

## The counter jumps by 100389 per telegram

That is correct. The counter counts the 32.768 kHz ticks of the receiver, not the telegrams.

## The board hangs and does not recover

A hang was observed once and could only be cleared by a manual reset — the sketch is not
self-recovering. The FH101RF has no wired reset (`RESET_N` open), so a foreign register access
cannot be undone from software.

This is why the shared-bus discipline in [Wiring](wiring) matters, and why both CS lines are
driven HIGH in `setup()` before any driver touches the bus.

## `arduino-cli upload` rejects `--build-property`

Correct, `upload` does not accept it. Put the switches in the sketch instead.

## IntelliSense reports missing `Arduino.h` / `SPI.h`

There is no Arduino include path configured for the workspace, so VS Code cannot resolve them
for `.ino` files. `arduino-cli` compiles clean — this is not a real error.
