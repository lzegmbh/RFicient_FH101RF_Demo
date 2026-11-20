/**
 * @file fh101rf_platform.h
 * @brief Arduino platform layer for the FH101RF driver (wurx.c / wurx.h).
 *
 * The driver itself is plain C. To call it from an Arduino sketch (.ino is
 * C++) its header is included with C linkage, otherwise the linker would look
 * for mangled C++ symbol names that wurx.c does not provide.
 */
#ifndef FH101RF_PLATFORM_H_
#define FH101RF_PLATFORM_H_

#include <Arduino.h>

#include "fh101rf_config.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "wurx.h"
#ifdef __cplusplus
}
#endif

/**
 * @brief initialize the platform: CS/IRQ/RESET pins, SPI master and a
 *        hardware reset of the FH101RF (if RESET_N is wired)
 */
void fh101rf_platform_begin(void);

#endif /* FH101RF_PLATFORM_H_ */
