# FH101RF Hardware/Software Example

A complete, ready-to-use hardware and software example for the **RFicient® FH101RF**
wake-up receiver. The project runs on an **Adafruit Feather M0 RFM69HCW** together with an
**FH101RFBOB** breakout board. It lets you configure the FH101RF and generate a matching RF
signal in FH101RF format, so a wake-up link can be built and tested without any additional
tooling.

```
   FH101RFBOB (receiver)                Feather M0 RFM69HCW (controller + transmitter)
   +--------------------+               +--------------------------------------+
   |  FH101RF           |<--- SPI ----->|  SAMD21                              |
   |  wake-up receiver  |<--- GPO1 -----|  RFM69HCW  --> OOK telegram          |
   +--------------------+               +--------------------------------------+
```

## What you get

| Function | Description |
|---|---|
| Configure the FH101RF | Band, branch, FDD, IDM, data rates, LO target, IRQ source and clock mode through the `wurx` driver |
| Calibrate the receiver | Crystal start-up, LO calibration and comparator offset calibration, each with a bounded timeout and a status flag |
| Generate the RF signal | The RFM69HCW transmits an OOK telegram in FH101RF format (31-bit CodeA preamble at the low data rate, CodeB data at the high data rate, ID block) |
| Receive and evaluate | The FH101RF reports an IRQ, the driver reads correlator state, correlator value, ID reason and FIFO payload into an event store |
| Store the events | Optional interrupt memory (`WURXIRQSTORE`) keeps up to `MAXLISTENTRIES` events with timestamp, band information and payload |
| Show the result | All state and event data is printed on the Feather serial console |

## Status

The link test was measured on real hardware and is verified:

```
link: 14 of 14 telegrams received
rx #N: IRQ_STATUS 0x20, ID individual, band 868, payload AF FE 00 00 00 00  -> payload OK
```

One event every 3062 ms, no dropouts. All four example sketches compile unchanged.

## Start here

| If you want to … | Read |
|---|---|
| See the whole link working in ten minutes | [Getting started](getting-started) |
| Understand the board and the antenna situation | [Hardware](hardware) |
| Order the hardware or get the data sheets | [Hardware](hardware#ordering-information) |
| Connect the BOB to the Feather | [Wiring](wiring) |
| Know what is on the air | [Signal format](signal-format) |
| Switch band or change the preset | [Configuration](configuration) |
| Pick the right sketch | [Sketches](sketches) |
| Integrate the driver into your own firmware | [Driver API](driver-api) |
| See the measured numbers | [Measurements](measurements) |
| Fix a link that does not work | [Troubleshooting](troubleshooting) |

## Repository layout

```
.
├── wurx.c                  FH101RF driver implementation
├── wurx.h                  FH101RF driver API, register map and enums
├── LICENSE                 MIT license
├── FH101RFRX/              Ready-to-use receiver sketch
├── FH101RFTX/              Transmitter library (portable C11) + two examples
├── FH101TRX/               Ready-to-use link test on a single board
├── docs/                   Photo of the test setup
└── wiki/                   Source of this wiki
```

The data sheets are not part of the repository, see [Hardware](hardware#data-sheets).

## License

MIT. The FH101RF data sheet and the FH101RFBOB data sheet are separate documents published by
LZE GmbH and are not covered by this license.
