/**
 * @file wurx_telegram.h
 * @brief FH101RF (RFicient) wake-up telegram generator.
 *
 * Generates the OOK chip stream of a FH101RF wake-up telegram for a
 * transmitter (e.g. an OOK capable transceiver or a simple OOK module).
 *
 * Telegram structure according to the FH101RF datasheet (section 5.6,
 * figure 6, table 16 and table 23):
 *
 *   1. Preamble     32-bit CodeA sequence, sent at the low data rate
 *                   (1024 bit/s with the 1k/32k preset). Every preamble bit
 *                   is held for WURX_TG_PREAMBLE_SPREAD chips, which turns it
 *                   into one bit at 1024 bit/s while the chip stream itself
 *                   runs at the high data rate.
 *   2. Pause        one code length of silence, the receiver switches from
 *                   LDR to HDR sampling. Must stay below TP1 = 5.371 ms at
 *                   32768 Hz, one code length is 0.977 ms.
 *   3. ID block     16-bit ID at the high data rate (32768 bit/s). Each ID
 *                   bit is coded as one CodeA chip (logical 0) or one CodeB
 *                   chip (logical 1), MSB first.
 *   4. Payload      optional data bytes at the high data rate, coded the same
 *                   way as the ID bits, MSB first per byte.
 *
 * The generated array is bit packed: bit 7 of byte 0 is the first chip, bit 0
 * of byte 0 the eighth chip and so on. A chip value of 1 means RF on, 0 means
 * RF off. Transmit the chips at WURX_TG_TX_CHIP_RATE_HZ, which is 1 % above
 * the receivers sampling rate as required by datasheet section 5.8.
 *
 * Example, 868 MHz preset of the wurx driver (wurx_conf_868_1k_32k_...):
 *   preamble 1 kbit/s, data 32 kbit/s, ID 0x7DA8 -> wurx_tg_generate(0x7DA8, 0, NULL)
 *
 * The module is plain C, so it can be used from C and C++ (Arduino) code.
 */
#ifndef WURX_TELEGRAM_H_
#define WURX_TELEGRAM_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief bytes of one coded bit, the 32-bit code sequence */
#define WURX_TG_CODE_BYTES 4u

/** @brief OOK chips of one coded bit */
#define WURX_TG_CODE_CHIPS (WURX_TG_CODE_BYTES * 8u)

/**
 * @brief preamble spread factor
 *
 * chip rate 32768 Hz / preamble rate 1024 Hz = 32. Every preamble bit is
 * transmitted as 32 identical chips, which is one bit at 1024 bit/s.
 */
#define WURX_TG_PREAMBLE_SPREAD 32u

/** @brief preamble length in chips: 32 code bits, each spread */
#define WURX_TG_PREAMBLE_CHIPS (WURX_TG_CODE_CHIPS * WURX_TG_PREAMBLE_SPREAD)

/**
 * @brief transmitter pause between preamble and ID block in chips
 *
 * One code length of silence (0.977 ms at 32768 chip/s). The receiver needs
 * this gap to switch from low data rate to high data rate sampling; it must
 * be shorter than TP1 = 5.371 ms.
 */
#define WURX_TG_PAUSE_CHIPS WURX_TG_CODE_CHIPS

/** @brief number of ID bits */
#define WURX_TG_ID_BITS 16u

/** @brief chip rate of the generated telegram (HDR sampling rate) in Hz */
#define WURX_TG_CHIP_RATE_HZ 32768u

/**
 * @brief recommended transmitter chip rate in Hz
 *
 * Datasheet section 5.8: fTX = 1.01 * 32768 Hz / 2^NFA, 1 % above the
 * receiver sampling rate. 1.01 * 32768 = 33096 Hz.
 */
#define WURX_TG_TX_CHIP_RATE_HZ 33096u

/** @brief ID of the FH101RFRX example sketch (wurx driver preset) */
#define WURX_TG_DEFAULT_ID 0x7DA8u

/**
 * @brief size of the internal buffer used by wurx_tg_generate()
 *
 * Required size in bytes is 196 for the ID-only telegram plus 32 bytes per
 * payload byte, i.e. 196 + 32 * size. The default of 388 bytes holds the ID
 * and a six byte payload. Use wurx_tg_build() with a caller supplied buffer
 * for longer payloads.
 */
#ifndef WURX_TG_BUFFER_SIZE
#define WURX_TG_BUFFER_SIZE 388u
#endif

/** @brief CodeA sequence (logical 0), 32-bit mlsA, datasheet table 23 */
extern const uint8_t wurx_tg_code_a[WURX_TG_CODE_BYTES];

/** @brief CodeB sequence (logical 1), 32-bit mlsB, datasheet table 23 */
extern const uint8_t wurx_tg_code_b[WURX_TG_CODE_BYTES];

/**
 * @brief number of chips of a telegram
 * @param size payload size in bytes, 0 = ID only
 * @retval number of chips, 0 if the telegram would not fit into 16 bit
 */
uint16_t wurx_tg_chips(uint8_t size);

/**
 * @brief number of bytes of a telegram
 * @param size payload size in bytes, 0 = ID only
 * @retval telegram size in bytes, 0 if the telegram would not fit into 16 bit
 */
uint16_t wurx_tg_size(uint8_t size);

/**
 * @brief generate a wake-up telegram into a caller supplied buffer
 * @param id        16-bit wake-up ID, transmitted MSB first
 * @param size      payload size in bytes, 0 = ID only
 * @param data      payload bytes, may be NULL if size is 0
 * @param telegram  output buffer, receives the bit packed chip stream
 * @param telegramSize size of the output buffer in bytes
 * @retval telegram size in bytes, 0 on error (buffer too small, data NULL
 *         with size > 0, telegram too long)
 */
uint16_t wurx_tg_build(uint16_t id, uint8_t size, const uint8_t *data,
                       uint8_t *telegram, uint16_t telegramSize);

/**
 * @brief generate a wake-up telegram into the internal static buffer
 * @param id   16-bit wake-up ID, transmitted MSB first
 * @param size payload size in bytes, 0 = ID only
 * @param data payload bytes, may be NULL if size is 0
 * @retval telegram size in bytes, 0 on error (payload does not fit into
 *         WURX_TG_BUFFER_SIZE)
 * @note use wurx_tg_get_telegram() to access the buffer
 */
uint16_t wurx_tg_generate(uint16_t id, uint8_t size, const uint8_t *data);

/**
 * @brief access the internal telegram buffer
 * @retval pointer to the buffer used by wurx_tg_generate()
 */
const uint8_t *wurx_tg_get_telegram(void);

/**
 * @brief read one chip out of a generated telegram
 * @param telegram telegram buffer
 * @param chipIndex chip index, 0 = first chip
 * @retval 1 = RF on, 0 = RF off
 */
uint8_t wurx_tg_get_chip(const uint8_t *telegram, uint16_t chipIndex);

#ifdef __cplusplus
}
#endif

#endif /* WURX_TELEGRAM_H_ */
