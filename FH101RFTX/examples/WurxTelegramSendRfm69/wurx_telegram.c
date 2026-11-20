/**
 * @file wurx_telegram.c
 * @brief FH101RF (RFicient) wake-up telegram generator.
 *
 * See wurx_telegram.h for the telegram structure.
 */
#include <string.h>

#include "wurx_telegram.h"

/* the preamble spreading and the pause are written byte wise, therefore both
   must be a multiple of 8 chips */
#if ((WURX_TG_PREAMBLE_SPREAD % 8u) != 0u)
#error "WURX_TG_PREAMBLE_SPREAD must be a multiple of 8"
#endif
#if ((WURX_TG_PAUSE_CHIPS % 8u) != 0u)
#error "WURX_TG_PAUSE_CHIPS must be a multiple of 8"
#endif
#if ((WURX_TG_CODE_CHIPS % 8u) != 0u)
#error "WURX_TG_CODE_CHIPS must be a multiple of 8"
#endif

/** CodeA, 32-bit mlsA sequence, datasheet table 23 */
const uint8_t wurx_tg_code_a[WURX_TG_CODE_BYTES] = {0x6A, 0xCD, 0x67, 0x4F};

/** CodeB, 32-bit mlsB sequence, datasheet table 23 */
const uint8_t wurx_tg_code_b[WURX_TG_CODE_BYTES] = {0x6D, 0x38, 0x97, 0x73};

/** internal buffer of wurx_tg_generate() */
static uint8_t wurx_tg_buffer[WURX_TG_BUFFER_SIZE];

/** size of the last telegram in wurx_tg_buffer, 0 = no valid telegram */
static uint16_t wurx_tg_buffer_size = 0;

/**
 * @brief append one coded bit to the telegram
 * @param telegram output buffer
 * @param pos byte position in the output buffer
 * @param code CodeA or CodeB sequence
 * @retval new byte position
 */
static uint16_t wurx_tg_put_code(uint8_t *telegram, uint16_t pos, const uint8_t *code)
{
	memcpy(&telegram[pos], code, WURX_TG_CODE_BYTES);
	return (uint16_t)(pos + WURX_TG_CODE_BYTES);
}

uint16_t wurx_tg_chips(uint8_t size)
{
	/* preamble + pause + ID block + payload, each data bit is one code */
	uint32_t chips = WURX_TG_PREAMBLE_CHIPS + WURX_TG_PAUSE_CHIPS;
	chips += (uint32_t)WURX_TG_ID_BITS * WURX_TG_CODE_CHIPS;
	chips += (uint32_t)size * 8u * WURX_TG_CODE_CHIPS;

	if(chips > 0xFFFFu)
	{
		return 0;
	}
	return (uint16_t)chips;
}

uint16_t wurx_tg_size(uint8_t size)
{
	uint16_t chips = wurx_tg_chips(size);

	if(chips == 0)
	{
		return 0;
	}
	return (uint16_t)((chips + 7u) / 8u);
}

uint16_t wurx_tg_build(uint16_t id, uint8_t size, const uint8_t *data,
                       uint8_t *telegram, uint16_t telegramSize)
{
	uint16_t needed;
	uint16_t pos = 0;
	uint8_t bit;

	if(telegram == NULL)
	{
		return 0;
	}
	if((size > 0) && (data == NULL))
	{
		return 0;
	}

	needed = wurx_tg_size(size);
	if((needed == 0) || (needed > telegramSize))
	{
		return 0;
	}

	/* all chips are 0 (RF off) unless they are set below, the pause is
	   already correct after this */
	memset(telegram, 0x00, needed);

	/* 1) preamble: CodeA at the low data rate, every code bit is held for
	      WURX_TG_PREAMBLE_SPREAD chips = one bit at 1024 bit/s */
	for(uint8_t codeByte = 0; codeByte < WURX_TG_CODE_BYTES; codeByte++)
	{
		for(bit = 0; bit < 8u; bit++)
		{
			if((wurx_tg_code_a[codeByte] & (uint8_t)(0x80u >> bit)) != 0u)
			{
				/* 32 chips are 4 bytes of 0xFF */
				for(uint8_t spreadByte = 0;
				    spreadByte < (WURX_TG_PREAMBLE_SPREAD / 8u);
				    spreadByte++)
				{
					telegram[pos + spreadByte] = 0xFFu;
				}
			}
			pos = (uint16_t)(pos + (WURX_TG_PREAMBLE_SPREAD / 8u));
		}
	}

	/* 2) pause: one code length of silence, the receiver switches to HDR.
	      The bytes are already zero, only the position has to move. */
	pos = (uint16_t)(pos + (WURX_TG_PAUSE_CHIPS / 8u));

	/* 3) ID block at the high data rate, MSB first */
	for(bit = 0; bit < WURX_TG_ID_BITS; bit++)
	{
		if((id & (uint16_t)(1u << (WURX_TG_ID_BITS - 1u - bit))) != 0u)
		{
			pos = wurx_tg_put_code(telegram, pos, wurx_tg_code_b);
		}
		else
		{
			pos = wurx_tg_put_code(telegram, pos, wurx_tg_code_a);
		}
	}

	/* 4) payload at the high data rate, MSB first per byte */
	for(uint8_t dataByte = 0; dataByte < size; dataByte++)
	{
		for(bit = 0; bit < 8u; bit++)
		{
			if((data[dataByte] & (uint8_t)(0x80u >> bit)) != 0u)
			{
				pos = wurx_tg_put_code(telegram, pos, wurx_tg_code_b);
			}
			else
			{
				pos = wurx_tg_put_code(telegram, pos, wurx_tg_code_a);
			}
		}
	}

	return needed;
}

uint16_t wurx_tg_generate(uint16_t id, uint8_t size, const uint8_t *data)
{
	wurx_tg_buffer_size = wurx_tg_build(id, size, data,
	                                    wurx_tg_buffer, sizeof(wurx_tg_buffer));
	return wurx_tg_buffer_size;
}

const uint8_t *wurx_tg_get_telegram(void)
{
	if(wurx_tg_buffer_size == 0)
	{
		return NULL;
	}
	return wurx_tg_buffer;
}

uint8_t wurx_tg_get_chip(const uint8_t *telegram, uint16_t chipIndex)
{
	if(telegram == NULL)
	{
		return 0;
	}
	return (uint8_t)((telegram[chipIndex >> 3] >> (7u - (chipIndex & 0x07u))) & 0x01u);
}
