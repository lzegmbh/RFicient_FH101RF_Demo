/**
 * @file fh101rf_config.h
 * @brief Hardware configuration of the FH101RF Arduino platform layer.
 *
 * All values can be overridden from outside (e.g. with a compiler define
 * -DFH101RF_CS_PIN=5 or by editing this file).
 */
#ifndef FH101RF_CONFIG_H_
#define FH101RF_CONFIG_H_

/**
 * SPI chip select of the FH101RF (BOB pin 2, CS_N, low active).
 * Pin 10 is the default SS pin of the Arduino Uno/Nano and is also free on
 * the Adafruit Feather M0 RFM69HCW (the on-board RFM69HCW uses D3/D4/D8).
 */
#ifndef FH101RF_CS_PIN
#define FH101RF_CS_PIN 10
#endif

/**
 * IRQ input connected to GPO1 (BOB pin 9). With the 868 MHz preset the
 * output mux is WURX_IRQ_EVENT_HIGH, so the IRQ signal is active high.
 */
#ifndef FH101RF_IRQ_PIN
#define FH101RF_IRQ_PIN 9
#endif

/**
 * Optional reset output connected to RESET_N (BOB pin 1, low active).
 * Set to -1 if the pin is not wired, the BOB has an internal pull-up.
 * For the wiring from the repository README (Feather M0) use 11.
 */
#ifndef FH101RF_RESET_PIN
#define FH101RF_RESET_PIN -1
#endif

/**
 * SPI clock in Hz. The FH101RF supports up to 10 MHz; 4 MHz is a robust
 * default for jumper wires. Increase it for short PCB traces if needed.
 */
#ifndef FH101RF_SPI_HZ
#define FH101RF_SPI_HZ 4000000UL
#endif

#endif /* FH101RF_CONFIG_H_ */
