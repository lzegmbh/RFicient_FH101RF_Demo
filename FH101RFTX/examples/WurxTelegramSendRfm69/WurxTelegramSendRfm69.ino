/**
 * WurxTelegramSendRfm69 - sends FH101RF wake-up telegrams with the RFM69HCW.
 *
 * Builds the telegram with the FH101RFTX library and transmits it with the
 * on-board RFM69HCW of the Adafruit Feather M0 RFM69HCW. The chip stream is
 * written into the RFM69 FIFO and the chip shifts it out at the programmed bit
 * rate. The bit timing therefore comes from the module crystal and is as exact
 * as the crystal allows, no bit banging in the main loop.
 *
 * The RFM69 must run in PACKET mode for this to work. In continuous mode the
 * FIFO is switched off entirely (datasheet 5.4.1: "The FIFO and packet handler
 * are thus inactive"), every FIFO write is discarded and the PA emits a flat
 * unmodulated carrier. PacketFormat = 0 with PayloadLength = 0 selects the
 * unlimited length packet format, which moves FIFO bytes to the modulator
 * unchanged because preamble and sync are disabled.
 *
 * Configuration:
 *   carrier frequency : 868.4 MHz   (TX_FREQ_HZ)
 *   chip rate         : 33096 Hz    (WURX_TG_TX_CHIP_RATE_HZ, 1 % above the
 *                                    receiver sampling rate of 32768 Hz)
 *   modulation        : OOK, packet mode with unlimited length
 *   CRC               : off
 *   address filter/ID : off
 *   preamble          : 0 bytes, minimal
 *   sync word         : off
 *
 * Note: the FH101RFBOB 868 MHz SAW filter is centered at 868.3 MHz. 868.4 MHz
 * is 100 kHz above the center, so the signal stays inside the passband. Set
 * TX_FREQ_HZ to 868300000 to sit exactly on the center.
 *
 * The matching receiver configuration is the FH101RFRX example sketch:
 * band 868 MHz, preamble 1 kbit/s, data 32 kbit/s, ID 0x7DA8,
 * ID match + slow event IRQ.
 */
#include "wurx_telegram.h"

#include "rfm69_ook.h"

/* -------------------------------------------------------------------------
 * configuration
 * ---------------------------------------------------------------------- */

/** carrier frequency in Hz */
#ifndef TX_FREQ_HZ
#define TX_FREQ_HZ 868400000UL
#endif

/** repeat interval in ms, 0 = only send on request via the serial console */
#ifndef TX_REPEAT_MS
#define TX_REPEAT_MS 2000UL
#endif

/** wake-up ID, must match ID_HI/ID_LO of the receiver */
#ifndef TX_ID
#define TX_ID WURX_TG_DEFAULT_ID
#endif

/**
 * payload bytes, sent after the ID at the high data rate.
 * Use size 0 to send the ID only.
 * Note: the receiver FIFO holds 16 bit = 2 bytes by default (register 0x56
 * FIFO_LENGTH), longer payloads need a different FIFO_LENGTH setting.
 */
static const uint8_t txPayload[] = {0xAF, 0xFE};
#define TX_PAYLOAD_SIZE ((uint8_t)sizeof(txPayload))

/** status LED, -1 disables it */
#ifdef LED_BUILTIN
#define TX_STATUS_LED_PIN LED_BUILTIN
#else
#define TX_STATUS_LED_PIN -1
#endif

/** send the telegram back to back, toggled with 't' / 'T' */
static bool repeatTelegram = false;

/**
 * A complete telegram has to be sent within one HDR phase of the receiver.
 * With 6 payload bytes the telegram is 3104 chips long, which is 93.8 ms.
 * The FIFO is fed by the main loop, so a telegram that is much longer than
 * that leaves the receiver in LDR mode and is not detected as one telegram.
 */
#if (WURX_TG_BUFFER_SIZE < (196u + 32u * 6u))
#warning "WURX_TG_BUFFER_SIZE is small, check the payload size"
#endif

/**
 * @brief build and transmit the configured telegram
 * @retval true if the telegram was sent
 */
static bool txSend(void)
{
	uint16_t chips = wurx_tg_chips(TX_PAYLOAD_SIZE);
	uint16_t sizeBytes = wurx_tg_generate(TX_ID, TX_PAYLOAD_SIZE, txPayload);
	const uint8_t *telegram = wurx_tg_get_telegram();

	if ((sizeBytes == 0) || (telegram == NULL) || (chips == 0))
	{
		Serial.println(F("ERROR: telegram does not fit into the internal buffer"));
		Serial.print(F("       needed "));
		Serial.print((unsigned int)wurx_tg_size(TX_PAYLOAD_SIZE));
		Serial.print(F(" byte(s), buffer "));
		Serial.println((unsigned int)WURX_TG_BUFFER_SIZE);
		return false;
	}

	if (rfm69_ook_bitrate_reg() == 0)
	{
		Serial.println(F("ERROR: RFM69 is not configured"));
		return false;
	}

#if TX_STATUS_LED_PIN >= 0
	digitalWrite(TX_STATUS_LED_PIN, HIGH);
#endif

	uint32_t durationUs = rfm69_ook_send(telegram, sizeBytes);

#if TX_STATUS_LED_PIN >= 0
	digitalWrite(TX_STATUS_LED_PIN, LOW);
#endif

	Serial.print(F("sent: ID 0x"));
	Serial.print(TX_ID, HEX);
	Serial.print(F(", "));
	Serial.print((unsigned int)TX_PAYLOAD_SIZE);
	Serial.print(F(" payload byte(s), "));
	Serial.print((unsigned int)sizeBytes);
	Serial.print(F(" byte(s), "));
	Serial.print(durationUs);
	Serial.println(F(" us"));

	return true;
}

void setup()
{
	Serial.begin(115200);

#if TX_STATUS_LED_PIN >= 0
	pinMode(TX_STATUS_LED_PIN, OUTPUT);
	digitalWrite(TX_STATUS_LED_PIN, LOW);
#endif

	uint32_t waitStart = millis();
	while (!Serial && (millis() - waitStart) < 2000UL)
	{
	}

	Serial.println();
	Serial.println(F("=================================================="));
	Serial.println(F(" FH101RF wake-up telegram sender, RFM69HCW OOK"));
	Serial.println(F("=================================================="));

	if (!rfm69_ook_begin(TX_FREQ_HZ, WURX_TG_TX_CHIP_RATE_HZ))
	{
		uint8_t version = rfm69_ook_version();

		Serial.print(F("RegVersion         : 0x"));
		if (version < 0x10u)
		{
			Serial.print('0');
		}
		Serial.print((unsigned int)version, HEX);
		Serial.println(F(" (expected 0x24)"));

		Serial.print(F("SPI clock tried    : "));
		Serial.print(rfm69_ook_spi_hz());
		Serial.println(F(" Hz"));

		Serial.print(F("register test      : "));
		Serial.println(rfm69_ook_spi_ok() ? F("OK, bus carries data") : F("FAILED, no data on MISO"));

		Serial.println();
		Serial.println(F("ERROR: RFM69HCW not found"));

		if (!rfm69_ook_spi_ok())
		{
			Serial.println(F("  the SPI bus itself does not work:"));
			Serial.println(F("  - is the board really a Feather M0 RFM69HCW?"));
			Serial.println(F("  - SCK = D24, MOSI = D23, MISO = D22, CS = D8, RESET = D4"));
			Serial.println(F("  - 3V3 and GND connected, antenna not required for the test"));
		}
		else
		{
			Serial.println(F("  the SPI bus works, but the device is not an RFM69:"));
			Serial.println(F("  - RFM69W/CW answer with 0x24 as well, an RFM69HW is fine"));
			Serial.println(F("  - an SX127x/LoRa module answers with 0x12"));
			Serial.println(F("  - check that the module is not held in reset (RESET = D4)"));
		}

		Serial.println();
		return;
	}

	Serial.print(F("carrier frequency  : "));
	Serial.print(rfm69_ook_frequency_hz());
	Serial.print(F(" Hz ("));
	Serial.print((unsigned long)(rfm69_ook_frequency_hz() / 1000UL));
	Serial.println(F(" kHz)"));

	Serial.print(F("chip rate          : "));
	Serial.print(rfm69_ook_bitrate_hz());
	Serial.print(F(" Hz (RegBitrate = "));
	Serial.print((unsigned int)rfm69_ook_bitrate_reg());
	Serial.println(F(")"));

	Serial.print(F("TX power           : "));
	Serial.print((int)RFM69_TX_POWER_DBM);
	Serial.println(F(" dBm (PA_BOOST / PA1)"));

	Serial.print(F("ID                 : 0x"));
	Serial.println(TX_ID, HEX);

	Serial.print(F("payload            : "));
	Serial.print((unsigned int)TX_PAYLOAD_SIZE);
	Serial.print(F(" byte(s):"));
	for (uint8_t i = 0; i < TX_PAYLOAD_SIZE; i++)
	{
		Serial.print(' ');
		if (txPayload[i] < 0x10u)
		{
			Serial.print('0');
		}
		Serial.print((unsigned int)txPayload[i], HEX);
	}
	Serial.println();

	Serial.print(F("telegram           : "));
	Serial.print((unsigned int)wurx_tg_size(TX_PAYLOAD_SIZE));
	Serial.print(F(" byte(s), "));
	Serial.print((unsigned int)wurx_tg_chips(TX_PAYLOAD_SIZE));
	Serial.println(F(" chips"));

	if (TX_REPEAT_MS > 0)
	{
		Serial.print(F("repeat interval    : "));
		Serial.print((unsigned long)TX_REPEAT_MS);
		Serial.println(F(" ms"));
	}
	else
	{
		Serial.println(F("repeat interval    : off"));
	}

	Serial.println();
	Serial.println(F("ready: 's' send one telegram, 't' repeat on, 'T' repeat off"));
	Serial.println();
}

void loop()
{
	static uint32_t lastSend = 0;
	static bool firstSend = true;
	static bool ready = false;

	/* the first call checks whether the radio answered during setup() */
	if (!ready)
	{
		if (rfm69_ook_bitrate_reg() == 0)
		{
			/* configuration failed, do not try to transmit */
			delay(1000);
			return;
		}
		ready = true;
	}

	if (Serial.available() > 0)
	{
		char c = (char)Serial.read();

		if ((c == 's') || (c == 'S'))
		{
			txSend();
			lastSend = millis();
			firstSend = false;
		}
		else if ((c == 't') || (c == 'T'))
		{
			/* repeat the telegram back to back. A 63 ms telegram every 2 s is
			   a 3 % duty cycle, which is easy to miss on an analyser. */
			repeatTelegram = (c == 't');
			Serial.println(repeatTelegram ? F("telegram repeat ON")
			                             : F("telegram repeat OFF"));
		}
	}

	if (repeatTelegram)
	{
		txSend();
		return;
	}

	/* periodic send, the first telegram is sent right after start-up */
#if TX_REPEAT_MS > 0
	if (firstSend || ((millis() - lastSend) >= TX_REPEAT_MS))
	{
		txSend();
		lastSend = millis();
		firstSend = false;
	}
#endif
}
