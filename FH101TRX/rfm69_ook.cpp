/**
 * @file rfm69_ook.c
 * @brief Minimal RFM69HCW driver for OOK continuous mode transmission.
 *
 * See rfm69_ook.h for the description. The driver is written in C so it can
 * be used from C and C++ translation units.
 */
#include <Arduino.h>
#include <SPI.h>

#include "rfm69_ook.h"

/* RFM69 SPI: mode 0, MSB first, 10 MHz is the device limit */
static SPISettings rfm69SpiSettings(8000000UL, MSBFIRST, SPI_MODE0);

/** programmed bit rate register, 0 = not configured */
static uint16_t rfm69BitrateReg = 0;

/** programmed carrier frequency register, 0 = not configured */
static uint32_t rfm69FrfReg = 0;

/** RegVersion value of the last rfm69_ook_begin() call */
static uint8_t rfm69Version = 0;

/** result of the register write/read-back test */
static bool rfm69SpiOk = false;

/** SPI clock that was used last */
static uint32_t rfm69SpiHz = 0;

/**
 * @brief write one RFM69 register, an SPI transaction must be open
 * @param addr register address (0x00..0x7F)
 * @param value value to write
 *
 * The RFM69 uses bit 7 of the address byte as direction: 1 = write,
 * 0 = read (opposite of the FH101RF).
 */
static void rfm69WriteRegRaw(uint8_t addr, uint8_t value)
{
	digitalWrite(RFM69_CS_PIN, LOW);
	delayMicroseconds(RFM69_CS_SETUP_US);
	SPI.transfer((uint8_t)(addr | 0x80));
	SPI.transfer(value);
	delayMicroseconds(RFM69_CS_HOLD_US);
	digitalWrite(RFM69_CS_PIN, HIGH);
}

/**
 * @brief read one RFM69 register, an SPI transaction must be open
 * @param addr register address (0x00..0x7F)
 * @retval register value
 */
static uint8_t rfm69ReadRegRaw(uint8_t addr)
{
	uint8_t value;

	digitalWrite(RFM69_CS_PIN, LOW);
	delayMicroseconds(RFM69_CS_SETUP_US);
	SPI.transfer((uint8_t)(addr & 0x7F));
	value = SPI.transfer((uint8_t)0x00);
	delayMicroseconds(RFM69_CS_HOLD_US);
	digitalWrite(RFM69_CS_PIN, HIGH);

	return value;
}

/**
 * @brief write one byte into the RFM69 FIFO, an SPI transaction must be open
 * @param value byte to send
 */
static void rfm69WriteFifoRaw(uint8_t value)
{
	digitalWrite(RFM69_CS_PIN, LOW);
	delayMicroseconds(RFM69_CS_SETUP_US);
	SPI.transfer((uint8_t)(RFM69_REG_FIFO | 0x80));
	SPI.transfer(value);
	delayMicroseconds(RFM69_CS_HOLD_US);
	digitalWrite(RFM69_CS_PIN, HIGH);
}

/**
 * @brief write one RFM69 register
 * @param addr register address (0x00..0x7F)
 * @param value value to write
 */
static void rfm69WriteReg(uint8_t addr, uint8_t value)
{
	SPI.beginTransaction(rfm69SpiSettings);
	rfm69WriteRegRaw(addr, value);
	SPI.endTransaction();
}

/**
 * @brief read one RFM69 register
 * @param addr register address (0x00..0x7F)
 * @retval register value
 */
static uint8_t rfm69ReadReg(uint8_t addr)
{
	uint8_t value;

	SPI.beginTransaction(rfm69SpiSettings);
	value = rfm69ReadRegRaw(addr);
	SPI.endTransaction();

	return value;
}

/**
 * @brief pulse the RFM69 reset pin and wait for the boot time
 *
 * The RESET input of the RFM69 is active HIGH. While it is HIGH the device
 * still answers SPI reads, but every SPI write is ignored and the registers
 * keep their reset defaults. The pin therefore has to be released to LOW and
 * must stay LOW during normal operation.
 */
static void rfm69Reset(void)
{
#if RFM69_RESET_PIN >= 0
	pinMode(RFM69_RESET_PIN, OUTPUT);

	digitalWrite(RFM69_RESET_PIN, HIGH);	/* assert reset, high active */
	delay(1);
	digitalWrite(RFM69_RESET_PIN, LOW);		/* release, keep it LOW */
	delay(10);								/* datasheet: 5 ms after reset */
#endif
}

bool rfm69_ook_begin(uint32_t frequencyHz, uint32_t bitRateHz)
{
	/* SPI clocks to try, fastest first. A slower clock is more tolerant
	   against long wires, breadboards and missing pull-ups. */
	static const uint32_t spiSpeeds[] = {8000000UL, 4000000UL, 1000000UL, 250000UL};
	static const uint8_t spiSpeedCount =
		(uint8_t)(sizeof(spiSpeeds) / sizeof(spiSpeeds[0]));

	uint32_t bitrateDivider;
	uint32_t frf;
	uint8_t index;
	uint8_t probe;

	if((frequencyHz == 0) || (bitRateHz == 0))
	{
		return false;
	}

	pinMode(RFM69_CS_PIN, OUTPUT);
	digitalWrite(RFM69_CS_PIN, HIGH);

	/* The SPI peripheral has to be started before the first transfer.
	   beginTransaction() alone does not enable it on SAMD targets, without
	   begin() every transfer reads back 0x00 and the version check fails. */
	SPI.begin();

	/* SPI.begin() may reconfigure pins, keep the module deselected */
	digitalWrite(RFM69_CS_PIN, HIGH);

	rfm69Reset();

	/* find the highest SPI clock at which the device answers */
	rfm69Version = 0;
	for(index = 0; index < spiSpeedCount; index++)
	{
		rfm69SpiHz = spiSpeeds[index];
		rfm69SpiSettings = SPISettings(rfm69SpiHz, MSBFIRST, SPI_MODE0);

		rfm69Version = rfm69ReadReg(RFM69_REG_VERSION);
		if(rfm69Version == RFM69_VERSION_EXPECTED)
		{
			rfm69SpiOk = true;
			break;
		}
	}

	if(rfm69Version != RFM69_VERSION_EXPECTED)
	{
		/* Diagnostic: does the bus carry data at all? Write two different
		   patterns into a scratch register and read them back. */
		rfm69SpiHz = 1000000UL;
		rfm69SpiSettings = SPISettings(rfm69SpiHz, MSBFIRST, SPI_MODE0);

		rfm69WriteReg(RFM69_REG_SYNCVALUE1, 0xA5);
		probe = rfm69ReadReg(RFM69_REG_SYNCVALUE1);
		rfm69SpiOk = (probe == 0xA5);

		rfm69WriteReg(RFM69_REG_SYNCVALUE1, 0x5A);
		probe = rfm69ReadReg(RFM69_REG_SYNCVALUE1);
		rfm69SpiOk = rfm69SpiOk && (probe == 0x5A);

		rfm69WriteReg(RFM69_REG_SYNCVALUE1, 0x00);

		return false;
	}

	/* configuration is done in standby */
	rfm69WriteReg(RFM69_REG_OPMODE, RFM69_OPMODE_STANDBY);

	/* RegBitrate = Fxosc / bit rate, the same formula applies to OOK */
	bitrateDivider = (RFM69_FXOSC_HZ + (bitRateHz / 2u)) / bitRateHz;
	if(bitrateDivider < 1u)
	{
		bitrateDivider = 1u;
	}
	if(bitrateDivider > 0xFFFFu)
	{
		bitrateDivider = 0xFFFFu;
	}
	rfm69BitrateReg = (uint16_t)bitrateDivider;
	rfm69WriteReg(RFM69_REG_BITRATEMSB, (uint8_t)(bitrateDivider >> 8));
	rfm69WriteReg(RFM69_REG_BITRATELSB, (uint8_t)(bitrateDivider & 0xFFu));

	/* RegFrf = f * 2^19 / Fxosc */
	frf = (uint32_t)(((uint64_t)frequencyHz << 19) / (uint64_t)RFM69_FXOSC_HZ);
	rfm69FrfReg = frf;
	rfm69WriteReg(RFM69_REG_FRFMSB, (uint8_t)((frf >> 16) & 0xFFu));
	rfm69WriteReg(RFM69_REG_FRFMID, (uint8_t)((frf >> 8) & 0xFFu));
	rfm69WriteReg(RFM69_REG_FRFLSB, (uint8_t)(frf & 0xFFu));

	/* OOK in packet mode, no modulation shaping (sharpest OOK edges).
	   Continuous mode would switch the FIFO off, see RFM69_DATAMODUL_OOK. */
	rfm69WriteReg(RFM69_REG_DATAMODUL, RFM69_DATAMODUL_OOK);

	/* no preamble, no sync word: the telegram carries its own preamble */
	rfm69WriteReg(RFM69_REG_PREAMBLEMSB, 0x00);
	rfm69WriteReg(RFM69_REG_PREAMBLELSB, 0x00);
	rfm69WriteReg(RFM69_REG_SYNCCONFIG, 0x00);

	/* no CRC, no address filtering, no AES, and PayloadLength = 0.

	   PayloadLength = 0 together with PacketFormat = 0 selects the unlimited
	   length packet format (datasheet 5.5.2.3). The packet handler then does
	   not count the bytes in the FIFO and, because the preamble length is 0
	   and SyncOn is 0, it prepends nothing either. The FIFO contents reach
	   the modulator bit for bit, which is exactly what an OOK telegram needs:
	   the telegram already contains its own preamble and its own coding. */
	rfm69WriteReg(RFM69_REG_PACKETCONFIG1, 0x00);
	rfm69WriteReg(RFM69_REG_PACKETCONFIG2, 0x00);
	rfm69WriteReg(RFM69_REG_PAYLOADLENGTH, 0x00);
	rfm69WriteReg(RFM69_REG_NODEADRS, 0x00);
	rfm69WriteReg(RFM69_REG_BROADCASTADRS, 0x00);
	rfm69WriteReg(RFM69_REG_AUTOMODES, 0x00);

	/* transmission starts as soon as the FIFO is not empty */
	rfm69WriteReg(RFM69_REG_FIFOTHRESH, RFM69_FIFOTHRESH_TXSTART_FIFONOTEMPTY);

	/* PA_BOOST output at RFM69_TX_POWER_DBM (0 dBm by default), shortest PA
	   ramp (10 us) for the sharpest OOK edges and widest bandwidth, clock
	   output off. RegDataModul already selects ModulationShaping = 00, so no
	   additional Gaussian or raised cosine shaping is applied. */
	rfm69WriteReg(RFM69_REG_PALEVEL, (uint8_t)RFM69_PA_LEVEL);
	rfm69WriteReg(RFM69_REG_PARAMP, (uint8_t)RFM69_PARAMP);
	rfm69WriteReg(RFM69_REG_OCP, RFM69_OCP_ON_95MA);
	rfm69WriteReg(RFM69_REG_DIOMAPPING1, 0x00);
	rfm69WriteReg(RFM69_REG_DIOMAPPING2, RFM69_CLKOUT_OFF);

	/* clear pending interrupt flags */
	(void)rfm69ReadReg(RFM69_REG_IRQFLAGS1);
	(void)rfm69ReadReg(RFM69_REG_IRQFLAGS2);

	return true;
}

uint32_t rfm69_ook_send(const uint8_t *data, uint16_t length)
{
	uint16_t index = 0;
	uint32_t start;
	uint32_t guard;
	uint32_t feedGuard;
	uint32_t feedTimeoutUs;
	uint32_t elapsedUs;
	uint32_t duration;

	if((data == NULL) || (length == 0) || (rfm69BitrateReg == 0))
	{
		return 0;
	}

	/* A single SPI transaction is opened for the whole transmission. On SAMD
	   every beginTransaction() reconfigures the SERCOM (disable, init, clock,
	   enable), which costs far more than the two bytes of a FIFO access. With
	   one transaction per byte the feed loop cannot keep up with the radio and
	   the FIFO runs dry. */
	SPI.beginTransaction(rfm69SpiSettings);

	(void)rfm69ReadRegRaw(RFM69_REG_IRQFLAGS2);

	/* enter TX and wait for the PLL to lock, the FIFO is still empty so the
	   modulator does not start yet and the measurement begins with the first
	   bit instead of the oscillator settling time */
	rfm69WriteRegRaw(RFM69_REG_OPMODE, RFM69_OPMODE_TX);

	guard = micros();
	while((rfm69ReadRegRaw(RFM69_REG_IRQFLAGS1) & RFM69_IRQFLAGS1_PLLLOCK) == 0)
	{
		if((micros() - guard) > RFM69_TX_TIMEOUT_US)
		{
			break;
		}
	}

	start = micros();

	/* keep the 66 byte FIFO fed until all bytes are handed over. The guard
	   time is twice the expected transmission time, so a module that does not
	   shift the FIFO out cannot block the caller forever. */
	feedGuard = start;
	feedTimeoutUs = ((uint32_t)length * 8UL * 1000000UL) / rfm69_ook_bitrate_hz();
	feedTimeoutUs = (feedTimeoutUs * 2UL) + 100000UL;

	while(index < length)
	{
		if((rfm69ReadRegRaw(RFM69_REG_IRQFLAGS2) & RFM69_IRQFLAGS2_FIFOFULL) == 0)
		{
			rfm69WriteFifoRaw(data[index]);
			index++;
		}
		else if((micros() - feedGuard) > feedTimeoutUs)
		{
			break;
		}
	}

	/* In packet mode the FIFO is drained at the programmed bit rate, so the
	   FIFO empty flag is a real indication that the modulator has consumed
	   the telegram. One byte is still in the shift register when the flag
	   clears, so a byte time is added on top.

	   The nominal duration is computed from the bit rate as well and used as
	   a lower bound: the transmitter must never be switched off before the
	   last bit left the modulator, and the two values normally agree to
	   within a byte time. */
	duration = ((uint32_t)length * 8UL * 1000000UL) / rfm69_ook_bitrate_hz();

	/* wait for the FIFO to run empty, but never longer than the nominal time */
	while((micros() - start) < duration)
	{
		if((rfm69ReadRegRaw(RFM69_REG_IRQFLAGS2) & RFM69_IRQFLAGS2_FIFONOTEMPTY) == 0)
		{
			break;
		}
	}

	/* keep TX running until the last bit left the modulator */
	elapsedUs = (uint32_t)(micros() - start);
	while(elapsedUs < duration)
	{
		delayMicroseconds(duration - elapsedUs);
		elapsedUs = (uint32_t)(micros() - start);
	}

	/* the power amplifier needs a moment to ramp down */
	delayMicroseconds(50UL);

	rfm69WriteRegRaw(RFM69_REG_OPMODE, RFM69_OPMODE_STANDBY);

	SPI.endTransaction();

	return duration;
}

void rfm69_ook_standby(void)
{
	rfm69WriteReg(RFM69_REG_OPMODE, RFM69_OPMODE_STANDBY);
}

bool rfm69_ook_set_power_dbm(int8_t dBm)
{
	uint8_t level;

	if((dBm < -18) || (dBm > 13))
	{
		return false;
	}

	/* RegPaLevel = PA output selection | OutputPower, OutputPower = dBm + 18 */
	level = (uint8_t)(RFM69_PA_PA1_BOOST | (uint8_t)(dBm + 18));

	rfm69WriteReg(RFM69_REG_PALEVEL, level);

	/* A missed SPI write would silently keep the previous power level, so the
	   value is read back instead of trusted. */
	return (rfm69ReadReg(RFM69_REG_PALEVEL) == level);
}

uint8_t rfm69_ook_power_reg(void)
{
	return rfm69ReadReg(RFM69_REG_PALEVEL);
}

uint8_t rfm69_ook_version(void)
{
	return rfm69Version;
}

bool rfm69_ook_spi_ok(void)
{
	return rfm69SpiOk;
}

uint32_t rfm69_ook_spi_hz(void)
{
	return rfm69SpiHz;
}

uint16_t rfm69_ook_bitrate_reg(void)
{
	return rfm69BitrateReg;
}

uint32_t rfm69_ook_bitrate_hz(void)
{
	if(rfm69BitrateReg == 0)
	{
		return 0;
	}
	return (RFM69_FXOSC_HZ + ((uint32_t)rfm69BitrateReg / 2u)) / (uint32_t)rfm69BitrateReg;
}

uint32_t rfm69_ook_frequency_hz(void)
{
	if(rfm69FrfReg == 0)
	{
		return 0;
	}
	return (uint32_t)(((uint64_t)rfm69FrfReg * (uint64_t)RFM69_FXOSC_HZ) >> 19);
}
