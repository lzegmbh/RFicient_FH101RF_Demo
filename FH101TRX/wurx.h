/**
 _____   ______  _        _               _
|  __  |  ____|(_)      (_)             | |
| |__) || |__    _   ___  _   ___  _ __  | |_
|  _  / |  __|  | | / __|| | / _ | '_  | __|
| |   | |     | || (__ | ||  __/| | | || |_
|_|  _|_|     |_| ___||_| ___||_| |_| __|

Wurx config code compatible with RFicient WakeUp IC dune4

* @file wurx.h
* @brief RFicient WakeUp IC control library, provides initializing and configuration methods
*/
#ifndef WURX_WURX_H_
#define WURX_WURX_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @name FH101RF registers
 */
///@{
#define	 NFA433_SLOW 0x00
#define	 NFA433_FAST 0x01
#define	 NFA868_SLOW 0x02
#define	 NFA868_FAST 0x03
#define	 NFA2G4_SLOW 0x04
#define	 NFA2G4_FAST 0x05
#define	 CALIB_STATUS 0x06
#define	 CALIB_CTRL 0x07
#define	 TSP_CTRL 0x08
#define	 N_SPG_TARGET 0x09
#define	 SPG_FREQ 0x0A
#define	 N_LCO_TARGET_433_8 0xB
#define	 N_LCO_TARGET_433_0 0xC
#define	 N_LCO_TARGET_868_8 0xD
#define	 N_LCO_TARGET_868_0 0xE
#define	 N_LCO_TARGET_2G4_8 0xF
#define	 N_LCO_TARGET_2G4_0 0x10
#define	 LCO_RANGE_433 0x11
#define	 LCO_RANGE_868 0x12
#define	 LCO_RANGE_2G4 0x13
#define	 LCO_FREQ_433_8 0x14
#define	 LCO_FREQ_433_0 0x15
#define	 LCO_FREQ_868_8 0x16
#define	 LCO_FREQ_868_0 0x17
#define	 LCO_FREQ_2G4_8 0x18
#define	 LCO_FREQ_2G4_0 0x19
#define	 COMPREF_W_433 0x1A
#define	 COMPREF_W_868 0x1B
#define	 COMPREF_W_2G4 0x1C
#define	 COMPREF_M_433 0x1D
#define	 COMPREF_M_868 0x1E
#define	 COMPREF_M_2G4 0x1F
#define	 COMPREF_S_433 0x20
#define	 COMPREF_S_868 0x21
#define	 COMPREF_S_2G4 0x22
#define	 D_CORNER_CTRL 0x23
#define	 BAND_BRANCH_CTRL 0x24
#define	 TESTBUF_CTRL 0x25
#define	 IFAMP_GAIN_CTRL 0x26
#define	 RX_ACTIVE_SELECT 0x27
#define	 CODE_SELECT 0x28
#define	 KORREL_THRESH_A 0x29
#define	 KORREL_THRESH_B 0x2A
#define	 KORREL_STATE 0x2B
#define	 KORREL_VAL 0x2C
#define	 FDD_ENABLE 0x2D
#define	 FDD_ACTIVE 0x2E
#define	 FO_QUIT 0x2F
#define	 FDD_EXIT_COND 0x30
#define	 IRQ_SELECT 0x31
#define	 IRQ_STATUS 0x32
#define	 IRQ_CLR 0x33
#define	 IRQ_SET 0x34
#define	 ID_HI 0x35
#define	 ID_LO 0x36
#define	 IDM_ENABLE 0x37
#define	 IDM_CTRL 0x38
#define	 IDM_CLR 0x39
#define	 IDM_BAND 0x3A
#define	 IDM_REASON 0x3B
#define	 RTC_SELECT 0x3C
#define	 RTC_STATUS 0x3D
#define	 RTC_CLR 0x3E
#define	 RTCSH0_THRESH_8 0x3F
#define	 RTCSH0_THRESH_0 0x40
#define	 RTCSH1_THRESH_8 0x41
#define	 RTCSH1_THRESH_0 0x42
#define	 RTCLG0_THRESH_32 0x43
#define	 RTCLG0_THRESH_24 0x44
#define	 RTCLG0_THRESH_16 0x45
#define	 RTCLG0_THRESH_8 0x46
#define	 RTCLG0_THRESH_0 0x47
#define	 RTCLG1_THRESH_32 0x48
#define	 RTCLG1_THRESH_24 0x49
#define	 RTCLG1_THRESH_16 0x4A
#define	 RTCLG1_THRESH_8 0x4B
#define	 RTCLG1_THRESH_0 0x4C
#define	 CYCLPRESC 0x4D
#define	 CYCLTOP_8 0x4E
#define	 CYCLTOP_0 0x4F
#define	 CNTR40_32 0x50
#define	 CNTR40_24 0x51
#define	 CNTR40_16 0x52
#define	 CNTR40_8 0x53
#define	 CNTR40_0 0x54
#define	 CNTR40_CLR 0x55
#define	 FIFO_LENGTH 0x56
#define	 FIFO_COUNT_433 0x57
#define	 FIFO_COUNT_868 0x58
#define	 FIFO_COUNT_2G4 0x59
#define	 RX_FIFO_5_433 0x5A
#define	 RX_FIFO_4_433 0x5B
#define	 RX_FIFO_3_433 0x5C
#define	 RX_FIFO_2_433 0x5D
#define	 RX_FIFO_1_433 0x5E
#define	 RX_FIFO_0_433 0x5F
#define	 RX_FIFO_5_868 0x60
#define	 RX_FIFO_4_868 0x61
#define	 RX_FIFO_3_868 0x62
#define	 RX_FIFO_2_868 0x63
#define	 RX_FIFO_1_868 0x64
#define	 RX_FIFO_0_868 0x65
#define	 RX_FIFO_5_2G4 0x66
#define	 RX_FIFO_4_2G4 0x67
#define	 RX_FIFO_3_2G4 0x68
#define	 RX_FIFO_2_2G4 0x69
#define	 RX_FIFO_1_2G4 0x6A
#define	 RX_FIFO_0_2G4 0x6B
#define	 ACTUAL_NFA_433 0x6C
#define	 ACTUAL_NFA_868 0x6D
#define	 ACTUAL_NFA_2G4 0x6E
#define	 ACTUAL_BANDSELECT 0x6F
#define	 GENPURP_0 0x70
#define	 GENPURP_1 0x71
#define	 GENPURP_2 0x72
#define	 XTAL_OSC_ENA 0x73
#define	 XTAL_OSC_BYP 0x74
#define	 MUX_D_OUT_SEL 0x75
#define	 LC_TG_ENA 0x76
#define	 XTAL_GOOD 0x77
#define	 COMP_THRESH_W 0x78
#define	 COMP_THRESH_M 0x79
#define	 COMP_THRESH_S 0x7A
#define	 OFFSETCAL_CTRL 0x7B
#define	 KORREL_SV_CLEAR 0x7C
#define	REG_125 0x7D
#define	REG_126 0x7E
#define	VERSION 0x7F
///@}

/**
 * @brief FH101RF ic version number
 */
#define WURXICVERSION 0x41

/**
 * @brief FH101RF FIFO depth in bytes
 */
#define WURXFIFODEPTH 0x06

/**
 * @brief max counter cycles to wait for an irq event to clear
 * 1000 cycles at 32,768 Hz equals 30.5 ms
 */
#define WURX_IRQ_TIMEOUT 1000u

/**
 * @brief max counter cycles to wait for the comparator offset calibration
 * datasheet states approximately 100 ms for comparator calibration,
 * 10000 cycles at 32,768 Hz equals 305 ms
 */
#define WURX_OFFSET_CAL_TIMEOUT 10000u

/**
 * @brief max counter cycles to wait for the oscillator (lco) calibration
 * datasheet states approximately 7 ms for oscillator calibration,
 * 2000 cycles at 32,768 Hz equals 61 ms
 */
#define WURX_RUN_LO_CAL_TIMEOUT 2000u

/**
 * @brief max counter cycles to wait for a stable crystal oscillator clock
 * 32768u cycles at 32,768 Hz equals 1s
 */
#define WURX_INIT_XTAL_TIMEOUT 32768u

/**
 * @brief reset default value of register 0x73 XTAL_OSC_ENA
 * built-in crystal oscillator enabled
 */
#define WURX_XTAL_OSC_ENA_DEFAULT 0x01

/**
 * @brief reset default value of register 0x74 LDO_XTAL_CTRL
 * LDO enabled, xtal bypass off, INT<1:0> = 0b11
 */
#define WURX_XTAL_OSC_BYP_DEFAULT 0x03


///FH101RF band selectors
typedef enum{
	WURX_BAND_ALL_OFF = 0x00,	///<All Bands off
	WURX_BAND_433 = 0x10, ///<Band 433 MHz on
	WURX_BAND_868 = 0x20, ///<Band 868 MHz on
	WURX_BAND_2G4 = 0x40, ///<Band 2G4 MHz on
}wurx_band_selector_t;

///FH101RF irq band events
typedef enum{
	WURX_BAND_IRQFIRED_433 = 0x00, ///<Band 433 MHz on that irq was fired
	WURX_BAND_IRQFIRED_868, ///<Band 868 MHz on that irq was fired
	WURX_BAND_IRQFIRED_2G4 ///<Band 2G4 MHz on that irq was fired
}wurx_band_irq_event_t;

///FH101RF branch selectors
typedef enum{
	WURX_BRANCH_ALL_OFF=0x00,	///< all branches off
	WURX_BRANCH_STRONG=0x04,	///< branch strong
	WURX_BRANCH_MEDIUM=0x02,	///< branch medium
	WURX_BRANCH_WEAK=0x01,		///< branch weak
}wurx_branch_selector_t;

///FH101RF data rates
typedef enum{
	WURX_32kbps=0x00,	///< 32768 bit/s
	WURX_16kbps,	///< 16384 bit/s
	WURX_8kbps,		///< 8192 bit/s
	WURX_4kbps,		///< 4096 bit/s
	WURX_2kbps,		///< 2048 bit/s
	WURX_1kbps,		///< 1024 bit/s
	WURX_512bps,	///< 512 bit/s
	WURX_256bps  	///< 256 bit/s
}wurx_drate_selector_t;

///FH101RF irq events
typedef enum{
	WURX_IDM_IRQ=1,			///< ID-Match IRQ-Event
	WURX_FIFO_OVF_IRQ=2,	///< FIFO overflow IRQ-Event
	WURX_FIFO_FULL_IRQ=4, 	///< FIFO filled IRQ-Event
	WURX_CODEAB_IRQ=8,		///< CodeA/B IRQ-Event
	WURX_IDMANDFIFO_IRQ=16,	///< IDM and FIFO combined IRQ-Event, both events must occur to fire irq
	WURX_IDMANDSLOW_IRQ=32,	///< IDM and SLOW IRQ-Event
	WURX_RTC_IRQ=64,		///< RTC alarm IRQ-Event
	WURX_RTCC_IRQ=128		///< cyclic RTC alarm IRQ-Event
}wurx_irq_event_t;

///FH101RF isr return states
typedef enum{
	WURX_ISR_OK = 0,            ///< irq event handled and cleared
	WURX_ISR_CLEAR_TIMEOUT,     ///< irq event did not clear within WURX_IRQ_TIMEOUT cycles
}wurx_isr_t;

///FH101RF timer types
typedef enum{
  WURX_RTCSH0=0x01,    ///< 16-bit short-term timer
  WURX_RTCSH1=0x02,    ///< 16-bit short-term timer
  WURX_RTCLG0=0x04,    ///< 40-bit long-term timer
  WURX_RTCLG1=0x08,    ///< 40-bit long-term timer
  WURX_CYCL=0x10,       ///< 16-bit cyclic timer
}wurx_timer_types_t;

///FH101RF xtal init mode
typedef enum{
  WURX_EXT_CLK,   ///< external clock on XI pin
  WURX_XTAL_CLK,  ///< xtal on XI and XO pins
  WURX_XTAL_NOT_MF, ///< xtal on XI and XO pins, IC version before metal fix
}wurx_clk_mode_t;

///FH101RF lo calibration events
typedef enum{
  WURX_LO_CAL_UNCALIBRATED,	  ///< no lo calibration performed yet
  WURX_LO_CAL_DONE,           ///< cal is done
  WURX_LO_CAL_IN_PROCSESS, ///< cal in process
  WURX_LO_CAL_ABORTED, ///< cal did not finish within WURX_RUN_LO_CAL_TIMEOUT
}wurx_lo_exit_event_t;

///FH101RF offset calibration events
typedef enum{
  WURX_OFFSET_CAL_UNCALIBRATED,   ///< no offset calibration performed yet
  WURX_OFFSET_CAL_DONE,           ///< cal is done
  WURX_OFFSET_CAL_IN_PROCSESS, ///< cal in process
  WURX_OFFSET_CAL_ABORTED, ///< cal did not finish within WURX_OFFSET_CAL_TIMEOUT
}wurx_offset_exit_event_t;

///FH101RF digital out selector
typedef enum{
	WURX_FAST_CLK_2G4_FAST_DATA_2G4, ///< RX data stream at 2.4 GHz
	WURX_FAST_CLK_868_FAST_DATA_868, ///< RX data stream at 868 MHz
	WURX_FAST_CLK_433_FAST_DATA_433, ///< RX data stream at 433 MHz
	WURX_WUP_A_2G4_WUP_B_2G4, ///< WakeUp signals at 2.4 GHz
	WURX_KOMP_OUT_W_RX_ACTIVE, ///< Test signals
	WURX_KOMP_OUT_M_KOMP_OUT_S, ///< Test signals
	WURX_ID_MATCH_WUP_A_433, ///< General ID match, WakeUp signal
	WURX_WUP_A_868_WUP_B_868, ///< WakeUp signals at 868 MHz
	WURX_WUP_A_433_RX_ACTIVE, ///< WakeUp signal, trigger signal
	WURX_WUP_A_868_RX_ACTIVE, ///< WakeUp signal, trigger signal
	WURX_WUP_A_2G4_RX_ACTIVE, ///< WakeUp signal, trigger signal
	WURX_KOMP_OUT_M_RX_ACTIVE, ///< Test signals
	WURX_KOMP_OUT_S_RX_ACTIVE, ///< Test signals
	WURX_KOMP_OUT_W_WUP_A_868, ///< Test signal, WakeUp signal
	WURX_CLK32_IRQ_EVENT, 	///< GPO1: system clock, GPO2: interrupt signal
	WURX_IRQ_EVENT_HIGH, ///< GPO1: interrupt signal, GPO2: static high
} wurx_dout_selector_t;

///FH101RF id match method
typedef enum{
	WURX_IDM_INDIVIDUAL=0x00,	///< only individual id
	WURX_IDM_INDIVIDUAL_GROUP=0x01, ///< individual id and Group id (high byte of id)
	WURX_IDM_BROADCAST=0x02,		///< only broadcast id 
	WURX_IDM_INDIVIDUAL_GROUP_BROADCAST=0x03,	///< individual id and Group id (high byte of id) + BROADCAST ID
}wurx_idmm_t;

///FH101RF id match reason
typedef enum{
	WURX_IDM_REASON_INDIVIDUAL=0x01,	///< individual id message
 	WURX_IDM_REASON_GROUP=0x02, ///< Group id (high byte of id) message
	WURX_IDM_REASON_BROADCAST=0x03,		///< broadcast id message
}wurx_idm_reason_t;

///FH101RF id match band selector
typedef enum{
	WURX_IDM_433=0x01,	///< IDM band 433 MHz
	WURX_IDM_868=0x02,	///< IDM band 868 MHz
	WURX_IDM_2G4=0x04,	///< IDM band 2G4 MHz
}wurx_idm_band_selector_t;

///FH101RF data decoder band selector
typedef enum{
	WURX_FDD_433=0x01,	///< FDD band 433 MHz
	WURX_FDD_868=0x02,	///< FDD band 868 MHz
	WURX_FDD_2G4=0x04,	///< FDD band 2G4 MHz
}wurx_fdd_band_selector_t;

///FH101RF configuration
typedef struct{
	wurx_band_selector_t band_selector;	///< band selector, enables corresponding band if set to one and disables if set to zero
	wurx_branch_selector_t branch_selector; ///< branch selector, enables corresponding branch if set to one and disables if set to zero
	wurx_fdd_band_selector_t fdd_selector; ///< fdd selector, enables corresponding fast data decoder if set to one and disables if set to zero
	wurx_idm_band_selector_t idm_selector;	///< idm selector, enables id detection feature of the IC
	wurx_drate_selector_t pream_data_rate_433; ///< set the data rate of the preamble receiver on 433 MHz
	wurx_drate_selector_t pream_data_rate_868; ///< set the data rate of the preamble receiver on 868 MHz
	wurx_drate_selector_t pream_data_rate_2G4; ///< set the data rate of the preamble receiver on 2G4 MHz
	wurx_drate_selector_t data_rate_433; ///< set the data rate of the data receiver on 433 MHz
	wurx_drate_selector_t data_rate_868; ///< set the data rate of the data receiver on 868 MHz
	wurx_drate_selector_t data_rate_2G4; ///< set the data rate of the data receiver on 2G4 MHz
	uint16_t lo_target_433;	///< lo frequency value for the 433 MHz band, it is not the carrier frequency
	uint16_t lo_target_868;	///< lo frequency value for the 868 MHz band, it is not the carrier frequency
	uint16_t lo_target_2g4;  ///< lo frequency value for the 2G4 MHz band, it is not the carrier frequency
	wurx_irq_event_t irq_source;	   ///< irq source, enable or disable wurx irq's
	wurx_idmm_t id_reason;	///< id type for idm detection
	uint16_t id;	///< set the id which is used by idm irq's
	wurx_dout_selector_t dout_mux;	///< set dout-pin muxer
	wurx_clk_mode_t wurx_clk_mode;  ///< set main clock (XI) feed
}wurx_config_t;


/**
 * @brief FH101RF device state
 */
typedef struct {
  bool receiverGood;                            ///< FH101RF receiver overall status
  wurx_offset_exit_event_t offsCalStatus;				///< comparator offset calibration status
	wurx_lo_exit_event_t loCalStatus;				    ///< lo calibration status
	bool xtalGood;					///< xtal oscillator status
	bool icIdGood;					  ///< IC status
	bool irqClearTimeout;				///< last irq event could not be cleared within WURX_IRQ_TIMEOUT
}wurx_state_t;

/**
 * @brief FH101RF irq event data container
 */
typedef struct
{
	wurx_irq_event_t irq_event:8;	          ///< triggered irq event
	wurx_idm_reason_t id_type:8;            ///< IDM type @irq event
	uint8_t correl_value;	                  ///< codeA/B correlator value @event, the value indicates the link quality
	uint8_t correl_state;	                  ///< correlator status @event
	uint64_t irq_time_stamp;                ///< irq timestamp, represents the FH101RF counter @irq event
	wurx_band_irq_event_t band_irq_fired:8;	///< band that event triggered
	uint8_t fifo_count;		                  ///< data count in bytes, when data was received on event
	uint8_t	fifo_data[WURXFIFODEPTH];	      ///< PAYLOAD data store
}__attribute__ ((packed)) wurx_irq_event_cont_t;


#ifdef WURXIRQSTORE
///max entries of wurx_irq_event_cont_t storage array
#define MAXLISTENTRIES 10

/**
 * @brief FH101RF irq event data container array where the FH101RF irq events are stored
 */
typedef struct
{
	uint8_t wurx_irqevent_store_counter; ///<count of stored entries
	wurx_irq_event_cont_t wurx_irqevent_store[MAXLISTENTRIES];  ///<wurx_irq_event store array
}__attribute__ ((packed)) wurx_irq_event_cont_store_t;
#endif

/**
 * @brief FH101RF configuration preset
 * single 433 MHz band
 * preamble data rate 1k and
 * irq fired on idm and slow
 * id=7da8, detects only individual id, no group and no broadcast
 *
 */
extern wurx_config_t wurx_conf_433_1k_32k_idm2slow_allbranch_dout1;

/**
 * @brief FH101RF configuration preset
 * single 868 MHz band
 * preamble data rate 1k and
 * irq fired on idm and slow
 * id=7da8, detects only individual id, no group and no broadcast
 *
 * @note for the 915 MHz band use wurx_conf_916_1k_32k_idm2slow_allbranch_dout1,
 *       it differs from this preset only in lo_target_868. That preset requires
 *       the nonEU board version, whose SAW filter in the shared 868/915 path is
 *       centered at 916.5 MHz; this preset requires the EU version, centered at
 *       868.3 MHz. The two bands are mutually exclusive on one board.
 */
extern wurx_config_t wurx_conf_868_1k_32k_idm2slow_allbranch_dout1;

/**
 * @brief FH101RF configuration preset
 * single 915 MHz band, uses the 868 MHz band path of the IC
 * preamble data rate 1k and
 * irq fired on idm and slow
 * id=7da8, detects only individual id, no group and no broadcast
 *
 * @note Hardware: the IC has no separate 915 MHz band path. 868 and 915 share
 *       one path, one matching network and one SMA connector; the board version
 *       decides which of the two is usable because only the SAW filter in that
 *       shared path is populated differently (FH101RFBOB_EU_V1.0: RF360
 *       B39871B3744H110, center 868.3 MHz / FH101RFBOB_nonEU_V1.0: RF360
 *       B39921B3300H110, center 916.5 MHz). 868 and 915 are therefore mutually
 *       exclusive on one board, and a frequency the board was not built for
 *       cannot be enabled by software.
 *
 * @note Software: this preset drives the 868 MHz band path and differs from
 *       wurx_conf_868_1k_32k_idm2slow_allbranch_dout1 in EXACTLY ONE value:
 *       lo_target_868 = 956 instead of 920. All other parameters are identical
 *       on purpose - if the 868 preset changes, change this one the same way.
 *       The nonEU filter is centered at 916.5 MHz, so 956 MHz LO gives an IF of
 *       39.5 MHz, essentially the datasheet optimum of 40 MHz (a carrier at
 *       exactly 915 MHz lands at 41 MHz).
 */
extern wurx_config_t wurx_conf_916_1k_32k_idm2slow_allbranch_dout1;

/**
 * @brief FH101RF configuration preset
 * single 2400 MHz band
 * preamble data rate 1k and
 * irq fired on idm and slow
 * id=7da8, detects only individual id, no group and no broadcast
 *
 */
extern wurx_config_t wurx_conf_2g4_1k_32k_idm2slow_allbranch_dout1;

/**
 * @brief perform an FH101RF initialization
 * @param config FH101RF configuration
 * @retval return wurx_state_t.receiverGood
 */
bool wurx_init(wurx_config_t config);

/**
 * @brief reset FH101RF registers to default values
 * @note weak default implementation is empty, override it to perform a real reset
 */
void wurx_reset_register(void) __attribute__ ((weak));

/**
 * @brief FH101RF isr handler
 * @param store pointer to the wurx_irq_event_cont_t entry that receives the event
 * @retval WURX_ISR_OK if the irq event was handled and cleared,
 *         WURX_ISR_CLEAR_TIMEOUT if the event did not clear within WURX_IRQ_TIMEOUT
 */
wurx_isr_t wurx_irq_handler(wurx_irq_event_cont_t *store);

#ifdef WURXIRQSTORE
/**
 * @brief FH101RF irq event store handling
 *
 * The store is the interrupt memory management of the wurx driver. It holds up
 * to MAXLISTENTRIES wurx_irq_event_cont_t entries, each one keeps the interrupt
 * data of a single event together with its timestamp and the band information.
 * If storing the interrupts and their environment parameters is required, this
 * functionality can be used. It is compiled in only if the preprocessor define
 * WURXIRQSTORE is set, so an application that does not need the store does not
 * pay for the RAM.
 *
 * Usage:
 * 1. the application takes the next free entry, e.g.
 *    &wurx_get_irq_event_array()->wurx_irqevent_store[count], and passes the
 *    pointer to wurx_irq_handler(), which fills the entry directly
 * 2. after a successful irq handling the application increments the counter
 *    itself, or it calls wurx_store_irq_event() with an already filled entry
 * 3. wurx_get_irq_event_counter() returns the number of stored entries,
 *    wurx_get_irq_event_last() the newest one, wurx_get_irq_event_array() the
 *    whole store
 * 4. wurx_clear_irq_count() resets the counter to zero after the entries were
 *    processed, all entries are free again
 *
 * @note wurx_irq_handler() writes unconditionally into the given entry and does
 *       not check the counter, therefore the caller has to keep the counter
 *       below MAXLISTENTRIES before it passes the entry pointer. Otherwise the
 *       write runs past the end of wurx_irqevent_store[].
 * @note the store is not volatile and not protected against concurrent access,
 *       the counter has to be updated in the same context in which the isr runs
 *       or the update has to be made atomic.
 */

/**
 * @brief access to wurx_irq_event_cont_store pointer
 * @retval return pointer to wurx_irq_event_cont_store
 */
wurx_irq_event_cont_store_t* wurx_get_irq_event_array(void);

/**
 * @brief get a pointer to the last irq entry
 * @retval pointer to the last entry, NULL if the store is empty
 */
wurx_irq_event_cont_t* wurx_get_irq_event_last(void);

/**
 * @brief check for unread wurx_irq_event_cont_store entries
 * @retval return count of wurx_irq_event_cont_store entries
 */
uint8_t wurx_get_irq_event_counter(void);

/**
 * @brief store the FH101RF irq event in the given wurx_irq_event_cont_store
 * @note  if the element count is >= MAXLISTENTRIES, on the next irq event all unread array elements will be overwritten
 * @param store pointer to the wurx_irq_event_cont_store that receives the event
 * @param event FH101RF irq event
 */
void wurx_store_irq_event(wurx_irq_event_cont_store_t *store, wurx_irq_event_cont_t event);

/**
 * @brief clear the wurx_irq_event_cont_store counter
 */
void wurx_clear_irq_count(void);
#endif

/**
 * @brief set the FH101RF data rate for preamble and data depend on selected band
 * @param pream preamble data rate
 * @param data  data rate of the payload, payload or id
 * @param band FH101RF band selector
 */
void wurx_set_data_rate(wurx_drate_selector_t pream, wurx_drate_selector_t data, wurx_band_selector_t band);

/**
 * @brief run to set FH101RF band and branch settings
 * @param[in] band FH101RF band selector
 * @param[in] branch  FH101RF branch selector
 */
void wurx_set_band_branch(uint8_t band, wurx_branch_selector_t branch);
/**
 * @brief perform an offset-cal
 * @retval device handler offsCalStatus
 */
wurx_offset_exit_event_t wurx_offset_cal(void);


/**
 * @brief perform a lo-cal
 * @retval device handler loCalStatus
 */
wurx_lo_exit_event_t wurx_run_lo_cal(void);

/**
 * @brief routine to initialize wurx xtal or clk32 feed
 * @param mode receiver clock mode
 */
void wurx_init_xtal(wurx_clk_mode_t mode);

/**
 * @brief set target frequency on selected band, that value is used by lo cal
 * The value must be different from the rf carrier on the selected band, for more information
 * read the data sheet. Example: 868 MHz band is in use, e.g. RF-Carrier=868 MHz, set_lo_target(WURX_BAND_868,910). IF=42 MHz
 * @param[in] band FH101RF band selector
 * @param[in] target target frequency in MHz
 * @retval true if lco target was set without issues, false otherwise
 */
bool wurx_set_lo_target(wurx_band_selector_t band, uint16_t target);

/**
 * @brief set or change irq source of FH101RF receiver
 * @param source of type wurx_irq_event_t
 */
void wurx_set_irq_source(wurx_irq_event_t source);

/**
 * @brief set rtc alarm
 * @param timer rtc timer to configure
 * @param alarm alarm value of the selected timer
 */
void wurx_set_rtc_alarm(wurx_timer_types_t timer, uint64_t alarm);

/**
 * @brief write a value in FH101RF register
 * @param addr register address
 * @param val register value
 */
void wurx_write_reg(uint8_t addr, uint8_t val) __attribute__ ((weak));

/**
 * @brief register read from FH101RF receiver
 * @param addr register address
 * @retval read register value
 */
uint8_t wurx_read_reg(uint8_t addr) __attribute__ ((weak));


#endif /* WURX_WURX_H_ */
