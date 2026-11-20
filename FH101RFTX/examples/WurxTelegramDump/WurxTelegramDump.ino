/**
 * WurxTelegramDump - demo for the FH101RFTX wake-up telegram generator.
 *
 * Builds FH101RF wake-up telegrams for the 868 MHz preset of the wurx driver
 * (preamble 1 kbit/s, data 32 kbit/s) and prints size, duration and content.
 * The generated chip stream is decoded back and compared with the input, so
 * the output also proves that the generator works.
 */
#include "wurx_telegram.h"

/* payload for the second telegram, use NULL/0 for an ID-only telegram */
static const uint8_t demoPayload[] = {0xAF, 0xFE, 0x12, 0x34, 0x56, 0x78};

/**
 * Optional transmission demo: set to the pin that drives the OOK transmitter.
 * -1 disables the demo, the telegram is only printed.
 */
#ifndef FH101RF_TX_DEMO_PIN
#define FH101RF_TX_DEMO_PIN -1
#endif

/**
 * @brief build a 32-bit word out of 32 consecutive chips
 */
static uint32_t readChipWord(const uint8_t *telegram, uint16_t chipIndex)
{
	uint32_t value = 0;

	for (uint8_t i = 0; i < 32u; i++)
	{
		value = (value << 1) | wurx_tg_get_chip(telegram, (uint16_t)(chipIndex + i));
	}
	return value;
}

/**
 * @brief build a 32-bit word out of a 4 byte code sequence
 */
static uint32_t codeWord(const uint8_t *code)
{
	return ((uint32_t)code[0] << 24) | ((uint32_t)code[1] << 16) |
	       ((uint32_t)code[2] << 8) | (uint32_t)code[3];
}

/**
 * @brief decode a generated telegram and compare it with the input
 * @retval true if the telegram matches id, size and data
 */
static bool verifyTelegram(uint16_t id, uint8_t size, const uint8_t *data,
                           const uint8_t *telegram)
{
	const uint32_t codeA = codeWord(wurx_tg_code_a);
	const uint32_t codeB = codeWord(wurx_tg_code_b);
	uint16_t chip = 0;
	uint16_t decodedId = 0;

	if (telegram == NULL)
	{
		return false;
	}

	/* preamble: 32 CodeA bits, every bit spread to 32 identical chips */
	for (uint8_t codeByte = 0; codeByte < WURX_TG_CODE_BYTES; codeByte++)
	{
		for (uint8_t bit = 0; bit < 8u; bit++)
		{
			uint8_t level = (uint8_t)((wurx_tg_code_a[codeByte] >> (7u - bit)) & 0x01u);

			for (uint8_t spread = 0; spread < WURX_TG_PREAMBLE_SPREAD; spread++)
			{
				if (wurx_tg_get_chip(telegram, chip) != level)
				{
					Serial.println(F("verify: preamble mismatch"));
					return false;
				}
				chip++;
			}
		}
	}

	/* pause: one code length of silence */
	for (uint8_t i = 0; i < WURX_TG_PAUSE_CHIPS; i++)
	{
		if (wurx_tg_get_chip(telegram, chip) != 0u)
		{
			Serial.println(F("verify: pause is not silent"));
			return false;
		}
		chip++;
	}

	/* ID block: 16 code chips, MSB first */
	for (uint8_t bit = 0; bit < WURX_TG_ID_BITS; bit++)
	{
		uint32_t word = readChipWord(telegram, chip);
		decodedId <<= 1;
		if (word == codeB)
		{
			decodedId |= 1u;
		}
		else if (word != codeA)
		{
			Serial.println(F("verify: invalid code in the ID block"));
			return false;
		}
		chip = (uint16_t)(chip + WURX_TG_CODE_CHIPS);
	}

	if (decodedId != id)
	{
		Serial.println(F("verify: ID mismatch"));
		return false;
	}

	/* payload: 8 code chips per byte, MSB first */
	for (uint8_t dataByte = 0; dataByte < size; dataByte++)
	{
		uint8_t decodedByte = 0;

		for (uint8_t bit = 0; bit < 8u; bit++)
		{
			uint32_t word = readChipWord(telegram, chip);
			decodedByte = (uint8_t)(decodedByte << 1);
			if (word == codeB)
			{
				decodedByte |= 1u;
			}
			else if (word != codeA)
			{
				Serial.println(F("verify: invalid code in the payload"));
				return false;
			}
			chip = (uint16_t)(chip + WURX_TG_CODE_CHIPS);
		}

		if (decodedByte != data[dataByte])
		{
			Serial.println(F("verify: payload mismatch"));
			return false;
		}
	}

	if (chip != wurx_tg_chips(size))
	{
		Serial.println(F("verify: telegram length mismatch"));
		return false;
	}

	return true;
}

/**
 * @brief print a telegram as hex dump
 */
static void printTelegram(const uint8_t *telegram, uint16_t size)
{
	for (uint16_t i = 0; i < size; i++)
	{
		if ((i % 16u) == 0u)
		{
			Serial.println();
			Serial.print(F("  "));
		}
		if (telegram[i] < 0x10u)
		{
			Serial.print('0');
		}
		Serial.print((unsigned int)telegram[i], HEX);
		Serial.print(' ');
	}
	Serial.println();
}

/**
 * @brief build, print and verify one telegram
 */
static void showTelegram(const __FlashStringHelper *name, uint16_t id,
                         uint8_t size, const uint8_t *data)
{
	uint16_t chips = wurx_tg_chips(size);
	uint16_t sizeBytes = wurx_tg_generate(id, size, data);
	const uint8_t *telegram = wurx_tg_get_telegram();

	Serial.println();
	Serial.print(F("--- "));
	Serial.print(name);
	Serial.println(F(" ---"));

	if ((sizeBytes == 0) || (telegram == NULL))
	{
		Serial.println(F("ERROR: telegram does not fit into the internal buffer"));
		Serial.println(F("       use wurx_tg_build() with a larger buffer"));
		return;
	}

	Serial.print(F("ID                 : 0x"));
	Serial.print(id, HEX);
	Serial.print(F(", payload "));
	Serial.print((unsigned int)size);
	Serial.println(F(" byte(s)"));

	Serial.print(F("telegram size      : "));
	Serial.print((unsigned int)sizeBytes);
	Serial.println(F(" byte(s)"));

	Serial.print(F("chips              : "));
	Serial.print((unsigned int)chips);
	Serial.print(F(", duration at "));
	Serial.print((unsigned long)WURX_TG_TX_CHIP_RATE_HZ);
	Serial.print(F(" Hz: "));
	Serial.print(((uint32_t)chips * 1000UL) / WURX_TG_TX_CHIP_RATE_HZ);
	Serial.println(F(" ms"));

	Serial.print(F("preamble           : "));
	Serial.print((unsigned int)(WURX_TG_PREAMBLE_CHIPS / WURX_TG_PREAMBLE_SPREAD));
	Serial.print(F(" bit at "));
	Serial.print((unsigned long)(WURX_TG_CHIP_RATE_HZ / WURX_TG_PREAMBLE_SPREAD));
	Serial.print(F(" bit/s, spread x"));
	Serial.println((unsigned int)WURX_TG_PREAMBLE_SPREAD);

	Serial.print(F("verify             : "));
	Serial.println(verifyTelegram(id, size, data, telegram) ? F("OK") : F("FAILED"));

	Serial.println(F("content:"));
	printTelegram(telegram, sizeBytes);
}

void setup()
{
	Serial.begin(115200);

	uint32_t waitStart = millis();
	while (!Serial && (millis() - waitStart) < 2000UL)
	{
	}

	Serial.println();
	Serial.println(F("=================================================="));
	Serial.println(F(" FH101RF wake-up telegram generator"));
	Serial.println(F("=================================================="));

	/* ID only, this is the telegram the FH101RFRX example listens for */
	showTelegram(F("ID only"), WURX_TG_DEFAULT_ID, 0, NULL);

	/* ID plus payload */
	showTelegram(F("ID and payload"), WURX_TG_DEFAULT_ID,
	             (uint8_t)sizeof(demoPayload), demoPayload);
}

void loop()
{
	/* nothing to do, the demo runs once in setup() */
}
