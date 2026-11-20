/**
 * @file fh101rf_platform.cpp
 * @brief Arduino implementation of the three FH101RF driver platform hooks:
 *        wurx_write_reg(), wurx_read_reg() and wurx_reset_register().
 *
 * SPI protocol (FH101RF datasheet, section 6.1):
 *   - SPI mode 0, MSB first, max. 10 MHz
 *   - CS_N low active, a transfer starts with the falling CS_N edge
 *   - header byte: bit 7 = 1 -> read, bit 7 = 0 -> write, bits 6..0 = address
 *   - write: header byte followed by the data byte
 *   - read : after the header byte the bidirectional SIO pin becomes an output
 *            and the register value is clocked out in the next 8 clocks
 *
 * Note: wurx.h is intentionally NOT included in this file. The header declares
 * the hooks with __attribute__((weak)); including it would make these
 * definitions weak as well and wurx.c already contains a weak default for
 * wurx_reset_register(). Without the include the definitions here are strong
 * and always win over the weak defaults in the driver.
 */
#include <Arduino.h>
#include <SPI.h>

#include "fh101rf_config.h"

/* SPI settings are static, the constructor only stores the values */
static const SPISettings fh101rfSpiSettings(FH101RF_SPI_HZ, MSBFIRST, SPI_MODE0);

/**
 * @brief write one FH101RF register via SPI
 * @param addr register address (0x00..0x7F)
 * @param val  value to write
 */
extern "C" void wurx_write_reg(uint8_t addr, uint8_t val)
{
	SPI.beginTransaction(fh101rfSpiSettings);

	digitalWrite(FH101RF_CS_PIN, LOW);
	/* header: read/write bit = 0 (write) + 7-bit register address */
	SPI.transfer((uint8_t)(addr & 0x7F));
	/* data byte, MSB first */
	SPI.transfer(val);
	digitalWrite(FH101RF_CS_PIN, HIGH);

	SPI.endTransaction();
}

/**
 * @brief read one FH101RF register via SPI
 * @param addr register address (0x00..0x7F)
 * @retval register value
 */
extern "C" uint8_t wurx_read_reg(uint8_t addr)
{
	uint8_t val;

	SPI.beginTransaction(fh101rfSpiSettings);

	digitalWrite(FH101RF_CS_PIN, LOW);
	/* header: read/write bit = 1 (read) + 7-bit register address */
	SPI.transfer((uint8_t)(addr | 0x80));
	/* dummy byte to clock out the register value, SIO switched to output */
	val = SPI.transfer((uint8_t)0x00);
	digitalWrite(FH101RF_CS_PIN, HIGH);

	SPI.endTransaction();

	return val;
}

/**
 * @brief reset the FH101RF to its chip default settings
 *
 * RESET_N is low active and has an internal pull-up on the breakout board,
 * so the reset is optional. If FH101RF_RESET_PIN is set to -1 this function
 * does nothing.
 */
extern "C" void wurx_reset_register(void)
{
#if FH101RF_RESET_PIN >= 0
	pinMode(FH101RF_RESET_PIN, OUTPUT);
	digitalWrite(FH101RF_RESET_PIN, HIGH);	/* idle level, RESET_N inactive */
	delay(1);
	digitalWrite(FH101RF_RESET_PIN, LOW);	/* reset pulse */
	delay(1);
	digitalWrite(FH101RF_RESET_PIN, HIGH);	/* release reset */
	delay(50);								/* boot time of the receiver */
#endif
}

/**
 * @brief initialize pins, SPI master and reset the FH101RF
 */
void fh101rf_platform_begin(void)
{
	/* CS_N must be high (inactive) before the SPI master is enabled */
	pinMode(FH101RF_CS_PIN, OUTPUT);
	digitalWrite(FH101RF_CS_PIN, HIGH);

	/* GPO1 is a push-pull output of the receiver, no pull-up required */
	pinMode(FH101RF_IRQ_PIN, INPUT);

#if FH101RF_RESET_PIN >= 0
	pinMode(FH101RF_RESET_PIN, OUTPUT);
	digitalWrite(FH101RF_RESET_PIN, HIGH);
#endif

	SPI.begin();

	/* bring the receiver into a defined state before it is configured */
	wurx_reset_register();
}
