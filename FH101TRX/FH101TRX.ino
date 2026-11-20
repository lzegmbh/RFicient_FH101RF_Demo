/**
 * FH101TRX - combined FH101RF wake-up link test on one board
 *
 * Merges the two projects into one sketch for link evaluation:
 *   TX: the on-board RFM69HCW sends the FH101RF wake-up telegram every
 *       TX_REPEAT_MS with the OOK packet-mode driver of the
 *       WurxTelegramSendRfm69 example.
 *   RX: the attached FH101RF breakout (FH101RFBOB) listens with the 868 MHz
 *       preset of the wurx driver, as in the FH101RFRX sketch.
 * Every received event is printed on the serial console together with a
 * payload comparison and link statistics (received/sent), so the link can
 * be watched directly on the PC while it runs.
 *
 * Measured result: the link works, 14 of 14 telegrams received. Note that the
 * on-board RFM69HCW of the Feather has no antenna fitted, which costs about
 * 18 dB - see TX_POWER_DBM.
 *
 * Hardware: Adafruit Feather M0 RFM69HCW + FH101RFBOB, one SPI bus:
 *   RFM69HCW (on-board): CS D8, RESET D4, SCK D24, MOSI D23, MISO D22
 *   FH101RFBOB         : CS_N -> D10, GPO1 -> D9, RESET_N open,
 *                        MOSI/SCLK/MISO shared with the RFM69
 * Both radios hang on the same SPI bus, so setup() drives both chip
 * selects to inactive (HIGH) before the first SPI transaction. A chip
 * whose CS floats while the other one is configured would clock in
 * foreign register writes, and the FH101RF has no wired reset to
 * recover from that (RESET_N open).
 *
 * TX: 868.4 MHz OOK, packet mode with unlimited length, 33096 chip/s,
 *     ID 0x7DA8, payload AF FE, TX_POWER_DBM (default +13 dBm, because the
 *     on-board RFM69 has no proper antenna fitted).
 * RX: band 868 MHz, all branches, preamble 1 kbit/s, data 32 kbit/s,
 *     ID 0x7DA8 (individual), IRQ on ID match + slow event, GPO1 = IRQ.
 *
 * Timing note: a telegram takes about 63 ms and the send call blocks for
 * that time. The FH101RF latches its IRQ until the event is read, so an
 * event that fires during the send is still picked up right after it.
 *
 * Shared SPI bus rules (both chips use SCK/MOSI/MISO):
 *   - exactly one chip may be selected at a time, the other CS_N has to be
 *     HIGH; spiBusIdle() enforces that before and after every access
 *   - the two chips use OPPOSITE direction bits in the address byte:
 *     RFM69 bit 7 = 1 means write, FH101RF bit 7 = 1 means read. A stray
 *     access to the wrong chip is therefore a valid command there and is
 *     executed instead of being ignored
 *   - rfm69_ook_send() holds one SPI transaction open for the whole
 *     telegram (63 ms), so nothing may touch the FH101RF inside that
 *     window and a latched event is read before the next send starts
 */

/* The transmit power is NOT set here. RFM69_TX_POWER_DBM is a compile time
   constant of rfm69_ook.cpp and a #define in this file would only change
   what is printed, not the PA register. The power is set at runtime with
   rfm69_ook_set_power_dbm() instead. */

#include <SPI.h>

#include "fh101rf_platform.h"
#include "wurx_telegram.h"
#include "rfm69_ook.h"

/* -------------------------------------------------------------------------
 * configuration
 * ---------------------------------------------------------------------- */

/** carrier frequency in Hz */
#ifndef TX_FREQ_HZ
#define TX_FREQ_HZ 868400000UL
#endif

/** send interval in ms */
#ifndef TX_REPEAT_MS
#define TX_REPEAT_MS 3000UL
#endif

/** wake-up ID, must match ID_HI/ID_LO of the receiver */
#ifndef TX_ID
#define TX_ID WURX_TG_DEFAULT_ID
#endif

/** payload bytes sent after the ID at the high data rate */
static const uint8_t txPayload[] = {0xAF, 0xFE};
#define TX_PAYLOAD_SIZE ((uint8_t)sizeof(txPayload))

/**
 * transmit enable
 *
 *  1 = the on-board RFM69HCW sends a telegram every TX_REPEAT_MS
 *  0 = the RFM69HCW is initialised and set to its power level, but stays
 *      silent. The receiver listens only, which is what a test with an
 *      external transmitter needs: the own carrier would otherwise block
 *      the receiver front end while it is on air.
 */
#ifndef TX_SEND_ENABLE
#define TX_SEND_ENABLE 1
#endif

/**
 * length of one listening window in ms
 *
 * The window is only a marker in the log: at its end the sketch prints how
 * many events arrived so far and keeps listening. This means a telegram that
 * is sent late is still received, which matters because the window starts when
 * the board boots and a human has to key the external transmitter.
 */
#ifndef RX_LISTEN_MS
#define RX_LISTEN_MS 10000UL
#endif

/**
 * transmit power in dBm, written into RegPaLevel at runtime
 *
 * Usable range is -18 .. +13 dBm on the PA_BOOST output. The on-board RFM69
 * has no antenna fitted, which costs roughly 18 dB of link budget: a power
 * sweep on this hardware showed the first receptions only at 0 dBm and above,
 * while the same receiver hears an external transmitter at full sensitivity.
 * The maximum level is therefore used so the link test works as delivered.
 * Fit an antenna (wire on the ANT pad or a u.FL antenna) and the level can go
 * back to something realistic like -10 dBm.
 */
#ifndef TX_POWER_DBM
#define TX_POWER_DBM 13
#endif

/* -------------------------------------------------------------------------
 * shared SPI bus
 * ---------------------------------------------------------------------- */

/**
 * @brief drive both chip selects to inactive
 *
 * Both radios share SCK/MOSI/MISO, so exactly one of them may be selected at
 * a time and neither may stay selected between two accesses. A selected chip
 * keeps driving MISO, and a floating CS lets a chip clock in whatever appears
 * on the bus as a command.
 *
 * The two chips also use OPPOSITE direction bits in the address byte: RFM69
 * bit 7 = 1 means write, FH101RF bit 7 = 1 means read. An access meant for
 * one chip is therefore a perfectly valid command for the other one and gets
 * executed instead of being ignored - so the deselected chip has to be
 * deselected properly, not just "probably".
 *
 * The function is idempotent and costs two digitalWrite() calls, so it is
 * called around every radio access in this sketch.
 */
static void spiBusIdle(void)
{
	digitalWrite(RFM69_CS_PIN, HIGH);
	digitalWrite(FH101RF_CS_PIN, HIGH);
}

/* -------------------------------------------------------------------------
 * link statistics
 * ---------------------------------------------------------------------- */

static uint32_t txCount = 0;   /* telegrams sent            */
static uint32_t rxCount = 0;   /* wake-up events received   */
static uint32_t rxLastAt = 0;  /* millis() of the last event */

/** power that is programmed in RegPaLevel by setup() */
static const int8_t txPower = TX_POWER_DBM;

/**
 * @brief build and transmit one telegram, print one status line
 */
static void txSend(void)
{
	uint16_t sizeBytes = wurx_tg_generate(TX_ID, TX_PAYLOAD_SIZE, txPayload);
	const uint8_t *telegram = wurx_tg_get_telegram();

	if ((sizeBytes == 0) || (telegram == NULL))
	{
		Serial.println(F("ERROR: telegram does not fit into the buffer"));
		return;
	}

	/* the FH101RF has to be deselected here: rfm69_ook_send() keeps one SPI
	   transaction open for the whole telegram */
	spiBusIdle();
	uint32_t durationUs = rfm69_ook_send(telegram, sizeBytes);
	spiBusIdle();
	txCount++;

	Serial.print(F("tx #"));
	Serial.print((unsigned long)txCount);
	Serial.print(F(": ID 0x"));
	Serial.print(TX_ID, HEX);
	Serial.print(F(", "));
	Serial.print((unsigned int)TX_PAYLOAD_SIZE);
	Serial.print(F(" payload byte(s), "));
	Serial.print((unsigned int)sizeBytes);
	Serial.print(F(" byte(s), "));
	Serial.print(durationUs);
	Serial.print(F(" us, "));
	Serial.print((int)txPower);
	Serial.println(F(" dBm"));
}

/**
 * @brief read the latched wake-up event and print it with a payload check
 */
static void rxHandleEvent(void)
{
	/* zero the entry first: wurx_irq_handler() does not fill fifo_data
	   when the event carries no payload */
	wurx_irq_event_cont_t event;
	memset(&event, 0, sizeof(event));

	/* the RFM69 has to be deselected while the FH101RF owns the bus */
	spiBusIdle();
	wurx_isr_t isrResult = wurx_irq_handler(&event);
	spiBusIdle();
	rxCount++;

	Serial.print(F("rx #"));
	Serial.print((unsigned long)rxCount);
	Serial.print(F(" after "));
	Serial.print(millis() - rxLastAt);
	Serial.print(F(" ms: IRQ_STATUS 0x"));
	Serial.print((unsigned int)event.irq_event, HEX);

	Serial.print(F(", ID "));
	switch (event.id_type)
	{
	case WURX_IDM_REASON_INDIVIDUAL:
		Serial.print(F("individual"));
		break;
	case WURX_IDM_REASON_GROUP:
		Serial.print(F("group"));
		break;
	case WURX_IDM_REASON_BROADCAST:
		Serial.print(F("broadcast"));
		break;
	default:
		Serial.print(F("none"));
		break;
	}

	Serial.print(F(", band "));
	switch (event.band_irq_fired)
	{
	case WURX_BAND_IRQFIRED_433:
		Serial.print(F("433"));
		break;
	case WURX_BAND_IRQFIRED_868:
		Serial.print(F("868"));
		break;
	case WURX_BAND_IRQFIRED_2G4:
		Serial.print(F("2.4G"));
		break;
	default:
		Serial.print(F("?"));
		break;
	}

	/* 40-bit FH101RF event counter, low 32 bit are enough here */
	Serial.print(F(", counter "));
	Serial.print((unsigned long)(event.irq_time_stamp & 0xFFFFFFFFUL));

	if (event.fifo_count > 0)
	{
		Serial.print(F(", payload "));
		for (uint8_t i = 0; (i < event.fifo_count) && (i < WURXFIFODEPTH); i++)
		{
			if (event.fifo_data[i] < 0x10)
			{
				Serial.print('0');
			}
			Serial.print((unsigned int)event.fifo_data[i], HEX);
			if (i + 1 < event.fifo_count)
			{
				Serial.print(' ');
			}
		}

		/* The payload has to start with what the TX side sends. The FH101RF
		   FIFO returns its whole depth, so the payload is followed by fill
		   bytes - only the leading bytes are meaningful. */
		bool payloadOk = (event.fifo_count >= TX_PAYLOAD_SIZE);
		for (uint8_t i = 0;
		     payloadOk && (i < TX_PAYLOAD_SIZE) && (i < WURXFIFODEPTH);
		     i++)
		{
			payloadOk = (event.fifo_data[i] == txPayload[i]);
		}
		Serial.println(payloadOk ? F(" -> payload OK")
		                         : F(" -> PAYLOAD MISMATCH"));
	}
	else
	{
		Serial.println(F(", no payload"));
	}

	/* with a silent transmitter there is no own telegram to compare
	   against, so the link ratio only makes sense when sending */
	if (txCount > 0)
	{
		Serial.print(F("   link: "));
		Serial.print((unsigned long)rxCount);
		Serial.print(F(" of "));
		Serial.print((unsigned long)txCount);
		Serial.println(F(" telegrams received"));
	}
	else
	{
		Serial.print(F("   external telegram #"));
		Serial.println((unsigned long)rxCount);
	}

	if (isrResult != WURX_ISR_OK)
	{
		Serial.println(F("WARNING: IRQ flag did not clear within the timeout"));
	}

	rxLastAt = millis();
}

/**
 * @brief receive-only listening window
 *
 * Runs when TX_SEND_ENABLE is 0: the transmitter is initialised but never
 * keyed, so an external transmitter can be received without the own carrier
 * sitting on top of it.
 *
 * A latched IRQ is handled as soon as it appears. The window is only a marker
 * in the log: at its end the sketch prints how many events arrived so far and
 * keeps listening, so a telegram that is keyed late is still received.
 */
static void rxListen(void)
{
	static uint32_t start = 0;
	static uint32_t lastReport = 0;
	static uint32_t window = 1;

	if (start == 0)
	{
		start = millis();
		lastReport = start;
	}

	/* handle a latched event immediately, not only at the next report */
	if (digitalRead(FH101RF_IRQ_PIN) == HIGH)
	{
		rxHandleEvent();
	}

	if ((millis() - lastReport) < RX_LISTEN_MS)
	{
		return;
	}
	lastReport = millis();

	Serial.print(F("--- window "));
	Serial.print((unsigned long)window);
	Serial.print(F(" over after "));
	Serial.print((unsigned long)((millis() - start) / 1000UL));
	Serial.print(F(" s, "));
	Serial.print((unsigned long)rxCount);
	Serial.println(F(" event(s) so far, still listening ---"));
	window++;
}

void setup()
{
	Serial.begin(115200);

	/* wait for the USB serial port, but do not block a board without USB */
	uint32_t waitStart = millis();
	while (!Serial && (millis() - waitStart) < 2000UL)
	{
	}

	Serial.println();
	Serial.println(F("=================================================="));
	Serial.println(F(" FH101TRX - FH101RF wake-up transceiver test"));
	Serial.println(F("=================================================="));

	/* ---------------------------------------------------------------
	 * shared SPI bus: bring it up ONCE, with both chip selects already
	 * inactive. SCK/MOSI/MISO are wired to both chips, so a CS that is
	 * still floating while the bus starts toggling lets that radio clock
	 * in register writes meant for the other one. The FH101RF cannot
	 * recover from that on its own: RESET_N is not wired, so there is no
	 * hardware reset once the bus is clean again.
	 *
	 * Both drivers call SPI.begin() themselves; doing it here first makes
	 * the bus state independent of which driver runs first and guarantees
	 * that no transfer happens with an undefined chip select level.
	 * -------------------------------------------------------------- */
	pinMode(RFM69_CS_PIN, OUTPUT);
	digitalWrite(RFM69_CS_PIN, HIGH);
	pinMode(FH101RF_CS_PIN, OUTPUT);
	digitalWrite(FH101RF_CS_PIN, HIGH);

	SPI.begin();
	spiBusIdle();

	/* ---------------------------------------------------------------
	 * transmitter: on-board RFM69HCW
	 * (its CS D8 is already inactive, see above)
	 * -------------------------------------------------------------- */
	if (!rfm69_ook_begin(TX_FREQ_HZ, WURX_TG_TX_CHIP_RATE_HZ))
	{
		Serial.print(F("ERROR: RFM69HCW not found (RegVersion 0x"));
		Serial.print((unsigned int)rfm69_ook_version(), HEX);
		Serial.println(F(", expected 0x24)"));
		Serial.println(F("       transmitter disabled, receiver continues"));
	}
	else
	{
		Serial.print(F("tx: RFM69HCW, "));
		Serial.print(rfm69_ook_frequency_hz());
		Serial.print(F(" Hz, "));
		Serial.print(rfm69_ook_bitrate_hz());
		Serial.print(F(" chip/s, PA_BOOST"));
		Serial.println();
	}

	/* set the power level and prove it by reading the register back */
	if (rfm69_ook_bitrate_reg() != 0)
	{
		spiBusIdle();
		bool powerOk = rfm69_ook_set_power_dbm(TX_POWER_DBM);
		spiBusIdle();

		Serial.print(F("tx: power "));
		Serial.print((int)TX_POWER_DBM);
		Serial.print(F(" dBm, RegPaLevel 0x"));
		Serial.print((unsigned int)rfm69_ook_power_reg(), HEX);
		Serial.println(powerOk ? F(" (verified)")
		                       : F(" (READ BACK MISMATCH)"));
	}

	Serial.print(F("tx: ID 0x"));
	Serial.print(TX_ID, HEX);
	Serial.print(F(", payload "));
	for (uint8_t i = 0; i < TX_PAYLOAD_SIZE; i++)
	{
		if (txPayload[i] < 0x10)
		{
			Serial.print('0');
		}
		Serial.print((unsigned int)txPayload[i], HEX);
		if (i + 1 < TX_PAYLOAD_SIZE)
		{
			Serial.print(' ');
		}
	}
	Serial.print(F(", every "));
	Serial.print((unsigned long)TX_REPEAT_MS);
	Serial.println(F(" ms"));

	Serial.print(F("tx: mode "));
#if TX_SEND_ENABLE
	Serial.println(F("sending"));
#else
	Serial.println(F("SILENT (RX only, external telegram test)"));
#endif

	/* ---------------------------------------------------------------
	 * receiver: FH101RFBOB
	 * -------------------------------------------------------------- */
	fh101rf_platform_begin();

	uint8_t icVersion = wurx_read_reg(0x7f);
	Serial.print(F("rx: IC version 0x"));
	Serial.println(icVersion, HEX);
	if (icVersion != WURXICVERSION)
	{
		Serial.println(F("rx: ERROR unexpected IC version, check the wiring"));
	}

	Serial.println(F("rx: wurx_init() with the 868 MHz preset"));
	Serial.println(F("    (xtal, LO and offset calibration)"));

	wurx_config_t rxConfig = wurx_conf_868_1k_32k_idm2slow_allbranch_dout1;

	spiBusIdle();
	uint32_t initStart = millis();
	bool receiverGood = wurx_init(rxConfig);
	uint32_t initTimeMs = millis() - initStart;
	spiBusIdle();

	/* lo_target_868 is the free running oscillator, not the carrier: an
	   868.4 MHz carrier lands at |868.4 - fLO| MHz intermediate frequency.
	   The preset uses 920 MHz, which gives 52 MHz. Moving it to 908 MHz
	   (IF 40 MHz, the datasheet optimum) was measured on this hardware and
	   made the link WORSE, so the preset value is kept. */
	uint16_t loUsed = rxConfig.lo_target_868;
	uint16_t ifMhz = (uint16_t)((loUsed > 868UL) ? (loUsed - 868UL) : (868UL - loUsed));
	Serial.print(F("rx: LO target "));
	Serial.print((unsigned int)loUsed);
	Serial.print(F(" MHz -> IF "));
	Serial.print((unsigned int)ifMhz);
	Serial.println(F(" MHz"));

	uint8_t xtalGood = wurx_read_reg(XTAL_GOOD);
	uint8_t calibStatus = wurx_read_reg(CALIB_STATUS);

	Serial.print(F("rx: init "));
	Serial.print(initTimeMs);
	Serial.print(F(" ms, XTAL_GOOD 0x"));
	Serial.print((unsigned int)xtalGood, HEX);
	Serial.print(F(", CALIB_STATUS 0x"));
	Serial.print((unsigned int)calibStatus, HEX);
	Serial.println(receiverGood ? F(", OK")
	                             : F(", FAILED - check XTAL and calibration"));

	/* read back the configuration the same way the FH101RFRX sketch does:
	   these four registers decide whether the receiver listens at all */
	uint8_t bandSelect = wurx_read_reg(ACTUAL_BANDSELECT);
	uint8_t bandBranch = wurx_read_reg(BAND_BRANCH_CTRL);
	uint8_t irqSelect = wurx_read_reg(IRQ_SELECT);
	uint8_t idHi = wurx_read_reg(ID_HI);
	uint8_t idLo = wurx_read_reg(ID_LO);

	Serial.print(F("rx: ACTUAL_BANDSELECT 0x"));
	Serial.print((unsigned int)bandSelect, HEX);
	Serial.println((bandSelect & 0x02) ? F(" (868 MHz active)")
	                                   : F(" (868 MHz NOT active)"));
	Serial.print(F("rx: BAND_BRANCH_CTRL 0x"));
	Serial.print((unsigned int)bandBranch, HEX);
	Serial.print(F(", IRQ_SELECT 0x"));
	Serial.print((unsigned int)irqSelect, HEX);
	Serial.print(F(", ID 0x"));
	Serial.print((unsigned int)idHi, HEX);
	Serial.println((unsigned int)idLo, HEX);

	Serial.println();
#if TX_SEND_ENABLE
	Serial.print(F("ready: sending every "));
	Serial.print((unsigned long)TX_REPEAT_MS);
	Serial.println(F(" ms, listening for wake-up events"));
#else
	Serial.println(F("ready: TX initialised but SILENT, receiver listening"));
	Serial.print(F("send the external telegram within the next "));
	Serial.print((unsigned long)(RX_LISTEN_MS / 1000UL));
	Serial.println(F(" s"));
#endif
	Serial.println();
}

void loop()
{
#if TX_SEND_ENABLE
	static uint32_t lastSend = 0;
	static bool firstSend = true;

	/* A latched event of the previous cycle is read before the next send:
	   rfm69_ook_send() blocks for the whole telegram and keeps the SPI
	   transaction open, so the FH101RF cannot be read during it. */
	if (digitalRead(FH101RF_IRQ_PIN) == HIGH)
	{
		rxHandleEvent();
	}

	/* send every TX_REPEAT_MS, the first telegram right away */
	if (firstSend || ((millis() - lastSend) >= TX_REPEAT_MS))
	{
		if (rfm69_ook_bitrate_reg() != 0)
		{
			txSend();
		}
		lastSend = millis();
		firstSend = false;
	}
#else
	/* transmit disabled: the RFM69HCW stays initialised but never keyed */
	rxListen();
#endif
}
