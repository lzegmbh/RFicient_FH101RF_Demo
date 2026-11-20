/**
 * FH101RFRX - FH101RF wake-up receiver, 868 MHz configuration
 *
 * Initializes the RFicient FH101RF (breakout board FH101RFBOB) with the
 * 868 MHz preset of the wurx driver library and then waits for wake-up
 * events on GPO1.
 *
 * The preset wurx_conf_868_1k_32k_idm2slow_allbranch_dout1 contains:
 *   - band           : 868 MHz, all branches (strong, medium, weak)
 *   - preamble (LDR) : 1 kbit/s
 *   - data     (HDR) : 32 kbit/s
 *   - LO target      : 920 MHz
 *   - ID             : 0x7DA8, individual ID only (no group, no broadcast)
 *   - IRQ source     : ID match + slow event (WURX_IDMANDSLOW_IRQ)
 *   - clock          : internal 32.768 kHz crystal oscillator
 *   - output mux     : GPO1 = IRQ event, GPO2 = static high
 *
 * Wiring (see FH101RFBOB documentation, pin header):
 *   BOB 2 CS_N   -> FH101RF_CS_PIN    (default D10)
 *   BOB 4 MOSI   -> MOSI              (hardware SPI)
 *   BOB 5 SCLK   -> SCK               (hardware SPI)
 *   BOB 3 MISO   -> MISO              (hardware SPI, 4-wire mode)
 *   BOB 9 GPO1   -> FH101RF_IRQ_PIN   (default D9)
 *   BOB 1 RESET_N-> FH101RF_RESET_PIN (optional, default not connected)
 *   BOB 6 VDD    -> 3V3               (1.6 V .. 3.63 V, must match MCU I/O)
 *   BOB 7 GND    -> GND
 *
 * The pin assignment is defined in fh101rf_config.h.
 */
#include "fh101rf_platform.h"

void setup()
{
	Serial.begin(115200);

	/*configure radio cs pin to output and set to high level*/
	pinMode(8, OUTPUT);
	digitalWrite(8, HIGH);

	/* wait for the USB serial port, but do not block a board without USB */
	uint32_t waitStart = millis();
	while (!Serial && (millis() - waitStart) < 2000UL)
	{
	}

	Serial.println();
	Serial.println(F("=================================================="));
	Serial.println(F(" FH101RF wake-up receiver - 868 MHz configuration"));
	Serial.println(F("=================================================="));

	/* pins, SPI master and optional hardware reset */
	fh101rf_platform_begin();

	/* check the SPI link before the driver runs its init sequence */
	uint8_t icVersion = wurx_read_reg(0x7f);
	Serial.print(F("IC version (reg 0x7F) : 0x"));
	Serial.println(icVersion, HEX);
	if (icVersion != WURXICVERSION)
	{
		Serial.println(F("ERROR: unexpected IC version."));
		Serial.println(F("       Check VDD, GND, CS_N, SCLK, MOSI/MISO."));
	}
	Serial.println();
	Serial.println(F("wurx_init() with 868 MHz preset:"));
	Serial.println(F("  band 868 MHz, all branches"));
	Serial.println(F("  preamble 1 kbit/s, data 32 kbit/s"));
	Serial.println(F("  LO target 920 MHz, ID 0x7DA8 (individual)"));
	Serial.println(F("  IRQ on ID match + slow event, GPO1 = IRQ"));
	Serial.println(F("  xtal calibration, LO calibration, offset calibration"));
	Serial.println(F("  (XTAL_GOOD needs about 0.5 s, init takes about 1 s)"));

	uint32_t initStart = millis();
	bool receiverGood = wurx_init(wurx_conf_868_1k_32k_idm2slow_allbranch_dout1);
	uint32_t initTimeMs = millis() - initStart;

	/* read back the state and configuration registers */
	uint8_t xtalGood = wurx_read_reg(XTAL_GOOD);
	uint8_t calibStatus = wurx_read_reg(CALIB_STATUS);
	uint8_t bandSelect = wurx_read_reg(ACTUAL_BANDSELECT);
	uint8_t bandBranch = wurx_read_reg(BAND_BRANCH_CTRL);
	uint8_t irqSelect = wurx_read_reg(IRQ_SELECT);
	uint8_t idHi = wurx_read_reg(ID_HI);
	uint8_t idLo = wurx_read_reg(ID_LO);

	Serial.println();
	Serial.print(F("init time             : "));
	Serial.print(initTimeMs);
	Serial.println(F(" ms"));

	Serial.print(F("XTAL_GOOD (0x77)      : "));
	Serial.println((xtalGood & 0x01) ? F("1, clock stable") : F("0, CLOCK NOT STABLE"));

	Serial.print(F("CALIB_STATUS (0x06)   : 0x"));
	Serial.println(calibStatus, HEX);

	Serial.print(F("ACTUAL_BANDSELECT     : 0x"));
	Serial.print(bandSelect, HEX);
	Serial.println((bandSelect & 0x02) ? F(" (868 MHz active)") : F(" (868 MHz NOT active)"));

	Serial.print(F("BAND_BRANCH_CTRL      : 0x"));
	Serial.println(bandBranch, HEX);

	Serial.print(F("IRQ_SELECT (0x31)     : 0x"));
	Serial.println(irqSelect, HEX);

	Serial.print(F("ID (0x35/0x36)        : 0x"));
	Serial.print(idHi, HEX);
	Serial.println(idLo, HEX);

	Serial.print(F("receiver state        : "));
	Serial.println(receiverGood ? F("OK") : F("FAILED"));

	if (receiverGood)
	{
		Serial.println(F("receiver is listening for 868 MHz wake-up telegrams"));
	}
	else
	{
		Serial.println(F("check XTAL_GOOD, the calibrations and the SPI wiring"));
	}
	Serial.println();
}

void loop()
{
	/* GPO1 is configured as WURX_IRQ_EVENT_HIGH, the IRQ signal is active high */
	if (digitalRead(FH101RF_IRQ_PIN) == HIGH)
	{
		/* zero the entry first: wurx_irq_handler() does not fill fifo_data
		   when the event carries no payload */
		wurx_irq_event_cont_t event;
		memset(&event, 0, sizeof(event));

		wurx_isr_t isrResult = wurx_irq_handler(&event);

		Serial.println();
		Serial.println(F("--- wake-up event ---"));

		Serial.print(F("IRQ_STATUS (0x32)  : 0x"));
		Serial.println((unsigned int)event.irq_event, HEX);

		Serial.print(F("IDM reason         : "));
		switch (event.id_type)
		{
		case WURX_IDM_REASON_INDIVIDUAL:
			Serial.println(F("individual ID"));
			break;
		case WURX_IDM_REASON_GROUP:
			Serial.println(F("group ID"));
			break;
		case WURX_IDM_REASON_BROADCAST:
			Serial.println(F("broadcast ID"));
			break;
		default:
			Serial.println(F("none"));
			break;
		}

		Serial.print(F("band               : "));
		switch (event.band_irq_fired)
		{
		case WURX_BAND_IRQFIRED_433:
			Serial.println(F("433 MHz"));
			break;
		case WURX_BAND_IRQFIRED_868:
			Serial.println(F("868 MHz"));
			break;
		case WURX_BAND_IRQFIRED_2G4:
			Serial.println(F("2.4 GHz"));
			break;
		default:
			Serial.println(F("unknown"));
			break;
		}

		/* 40-bit FH101RF counter at the time of the event */
		Serial.print(F("counter low 32 bit : "));
		Serial.println((unsigned long)(event.irq_time_stamp & 0xFFFFFFFFUL));

		if (event.fifo_count > 0)
		{
			Serial.print(F("FIFO bytes         : "));
			Serial.println((unsigned int)event.fifo_count);
			Serial.print(F("FIFO data          : "));
			for (uint8_t i = 0; (i < event.fifo_count) && (i < WURXFIFODEPTH); i++)
			{
				if (event.fifo_data[i] < 0x10)
				{
					Serial.print('0');
				}
				Serial.print((unsigned int)event.fifo_data[i], HEX);
				Serial.print(' ');
			}
			Serial.println();
		}

		if (isrResult != WURX_ISR_OK)
		{
			Serial.println(F("WARNING: IRQ flag did not clear within the timeout"));
		}
	}

	/* simple heartbeat so an idle receiver is visible on the console */
	static uint32_t lastHeartbeat = 0;
	if ((millis() - lastHeartbeat) >= 10000UL)
	{
		lastHeartbeat = millis();
		Serial.println(F("listening for 868 MHz wake-up telegrams ..."));
	}
}
