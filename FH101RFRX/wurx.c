/**
 _____   ______  _        _               _
|  __  |  ____|(_)      (_)             | |
| |__) || |__    _   ___  _   ___  _ __  | |_
|  _  / |  __|  | | / __|| | / _ | '_  | __|
| |   | |     | || (__ | ||  __/| | | || |_
|_|  _|_|     |_| ___||_| ___||_| |_| __|

Wurx config code compatible with RFicient WakeUp IC dune4

* @file wurx.c
* @brief RFicient WakeUp IC control library, provides initializing and configuration methods
*
*/
#include "wurx.h"

/*FH101RF device state*/
static wurx_state_t wurx_state={
                        .receiverGood = false,
                        .loCalStatus = WURX_LO_CAL_UNCALIBRATED,
                        .offsCalStatus = WURX_OFFSET_CAL_UNCALIBRATED,
                        .icIdGood = false,
                        .xtalGood = false,
                        .irqClearTimeout = false,
};


wurx_config_t wurx_conf_433_1k_32k_idm2slow_allbranch_dout1= {
											  .band_selector=WURX_BAND_433,
											  .branch_selector=WURX_BRANCH_STRONG|WURX_BRANCH_MEDIUM|WURX_BRANCH_WEAK,
											  .fdd_selector=WURX_FDD_433,
											  .idm_selector=WURX_IDM_433,
											  .pream_data_rate_433=WURX_1kbps,
											  .data_rate_433=WURX_32kbps,
											  .lo_target_433=470,
											  .irq_source=WURX_IDMANDSLOW_IRQ,
											  .id_reason=WURX_IDM_INDIVIDUAL,
											  .id=0x7da8,
											  .dout_mux=WURX_IRQ_EVENT_HIGH,
											  .wurx_clk_mode=WURX_XTAL_CLK

};

wurx_config_t wurx_conf_868_1k_32k_idm2slow_allbranch_dout1= {
											  .band_selector=WURX_BAND_868,
											  .branch_selector=WURX_BRANCH_STRONG|WURX_BRANCH_MEDIUM|WURX_BRANCH_WEAK,
											  .fdd_selector=WURX_FDD_868,
											  .idm_selector=WURX_IDM_868,
											  .pream_data_rate_868=WURX_1kbps,
											  .data_rate_868=WURX_32kbps,
											  .lo_target_868=920,
											  .irq_source=WURX_IDMANDSLOW_IRQ,
											  .id_reason= WURX_IDM_INDIVIDUAL,
											  .id=0x7da8,
											  .dout_mux=WURX_IRQ_EVENT_HIGH,
											  .wurx_clk_mode=WURX_XTAL_CLK

};



/*
 * 915 MHz band setting, meant for the nonEU variant of the FH101RFBOB.
 *
 * HARDWARE BACKGROUND: the IC has three band paths (433, 868, 2G4) and no
 * separate 915 MHz path - 868 and 915 share one path, one matching network and
 * one SMA connector. The board version decides which of the two is usable,
 * because only the SAW filter in that shared path is populated differently:
 *
 *   FH101RFBOB_EU_V1.0    RF360 B39871B3744H110, center  868.3 MHz
 *   FH101RFBOB_nonEU_V1.0 RF360 B39921B3300H110, center  916.5 MHz
 *
 * 868 and 915 are therefore mutually exclusive on one board. A frequency the
 * board was not built for cannot be enabled by software. Note that the nonEU
 * filter is centered at 916.5 MHz, not 915 MHz.
 *
 * SOFTWARE: this preset drives the 868 MHz band path and differs from
 * wurx_conf_868_1k_32k_idm2slow_allbranch_dout1 in EXACTLY ONE value:
 *
 *     lo_target_868 = 956   (868 MHz preset uses 920)
 *
 * Every other parameter is identical to the 868 MHz preset on purpose. If that
 * preset is ever changed, change this one the same way - lo_target_868 is the
 * only field that is allowed to differ.
 *
 * Intermediate frequency: 956 MHz LO against the 916.5 MHz filter center gives
 * 39.5 MHz, essentially the 40 MHz optimum of the datasheet; a carrier at
 * exactly 915 MHz lands at 41 MHz. Both are inside the 30..50 MHz window.
 */
wurx_config_t wurx_conf_916_1k_32k_idm2slow_allbranch_dout1= {
											  .band_selector=WURX_BAND_868,
											  .branch_selector=WURX_BRANCH_STRONG|WURX_BRANCH_MEDIUM|WURX_BRANCH_WEAK,
											  .fdd_selector=WURX_FDD_868,
											  .idm_selector=WURX_IDM_868,
											  .pream_data_rate_868=WURX_1kbps,
											  .data_rate_868=WURX_32kbps,
											  .lo_target_868=956,
											  .irq_source=WURX_IDMANDSLOW_IRQ,
											  .id_reason= WURX_IDM_INDIVIDUAL,
											  .id=0x7da8,
											  .dout_mux=WURX_IRQ_EVENT_HIGH,
											  .wurx_clk_mode=WURX_XTAL_CLK

};



wurx_config_t wurx_conf_2g4_1k_32k_idm2slow_allbranch_dout1= {
											  .band_selector=WURX_BAND_2G4,
											  .branch_selector=WURX_BRANCH_STRONG|WURX_BRANCH_MEDIUM|WURX_BRANCH_WEAK,
											  .fdd_selector=WURX_FDD_2G4,
											  .idm_selector=WURX_IDM_2G4,
											  .pream_data_rate_2G4=WURX_1kbps,
											  .data_rate_2G4=WURX_32kbps,
											  .lo_target_2g4=2440,
											  .irq_source=WURX_IDMANDSLOW_IRQ,
											  .id_reason=WURX_IDM_INDIVIDUAL,
											  .id=0x7da8,
											  .dout_mux=WURX_IRQ_EVENT_HIGH,
											  .wurx_clk_mode=WURX_XTAL_CLK

};

#ifdef WURXIRQSTORE
/**
 * @brief default irq event store instance
 *
 * Interrupt memory management of the wurx driver, only compiled if WURXIRQSTORE
 * is defined. It stores up to MAXLISTENTRIES irq events, each entry keeps the
 * interrupt data together with its timestamp and the band information. Use it
 * if the interrupts and their environment parameters have to be kept.
 *
 * The store occupies 1 + MAXLISTENTRIES * sizeof(wurx_irq_event_cont_t) bytes
 * of RAM, that is 201 bytes with the default settings (20 bytes per entry,
 * 10 entries).
 *
 * The store is written from the isr context and read from the main context:
 * - it is not declared volatile, so the compiler may cache the counter and the
 *   entries. If the isr and the main loop run on different contexts, the entry
 *   pointer and the counter must be handled by the application in an atomic or
 *   isr-safe way.
 * - wurx_irq_handler() does not check the counter. The caller has to keep
 *   wurx_irqevent_store_counter below MAXLISTENTRIES before it passes an entry
 *   pointer, otherwise the write runs past the end of wurx_irqevent_store[].
 * - wurx_clear_irq_count() resets the counter and makes all entries reusable.
 */
wurx_irq_event_cont_store_t wurx_irq_event_cont_store;
#endif

/**
 * @brief read the 40 bit FH101RF counter, high byte first
 * @retval counter value masked to 40 bit
 */
static uint64_t wurx_read_counter(void)
{
	/*note: the read register value is promoted to int, a shift by 32 would be
	  undefined behaviour on a 32 bit int. Cast to uint64_t before shifting.*/
	uint64_t value = 0;
	value |= ((uint64_t)wurx_read_reg(CNTR40_32) << 32);
	value |= ((uint64_t)wurx_read_reg(CNTR40_24) << 24);
	value |= ((uint64_t)wurx_read_reg(CNTR40_16) << 16);
	value |= ((uint64_t)wurx_read_reg(CNTR40_8)  <<  8);
	value |= ((uint64_t)wurx_read_reg(CNTR40_0));
	/*mask to 40 bit, keeps the difference calculation valid on counter wrap*/
	return value & 0xFFFFFFFFFFULL;
}

/**
 * @brief poll a register bit until it reaches the expected state
 * @param addr register address
 * @param mask bit mask to poll
 * @param wait_set true: wait until the masked bit is set, false: wait until cleared
 * @param timeout max counter cycles to wait
 * @retval true if the expected state was reached, false on timeout
 */
static bool wurx_wait_reg_bit(uint8_t addr, uint8_t mask, bool wait_set, uint32_t timeout)
{
	uint64_t start = wurx_read_counter();
	while(1)
	{
		if( ((wurx_read_reg(addr) & mask) != 0) == wait_set )
		{
			return true;
		}
		/*abort if the expected state was not reached within the timeout*/
		if( ((wurx_read_counter() - start) & 0xFFFFFFFFFFULL) > (uint64_t)timeout )
		{
			return false;
		}
	}
}


wurx_offset_exit_event_t wurx_offset_cal(void)
{
	/*check if cal is already in process*/
	if(wurx_read_reg(CALIB_STATUS)!=0x00)
	{
		wurx_state.offsCalStatus=WURX_OFFSET_CAL_IN_PROCSESS;
		return wurx_state.offsCalStatus;
	}


	/*save receiver register control values which will be changed by offset_cal*/
	uint8_t bandBranchSelValue=wurx_read_reg(BAND_BRANCH_CTRL);
	uint8_t calSelValue=wurx_read_reg(CALIB_CTRL);

	/*set all bands and branches on*/
	wurx_write_reg(BAND_BRANCH_CTRL,WURX_BAND_2G4+WURX_BAND_868+WURX_BAND_433+WURX_BRANCH_STRONG+WURX_BRANCH_MEDIUM+WURX_BRANCH_WEAK);
	/*set cal cfg to offset and run cal*/
	wurx_write_reg(CALIB_CTRL,0x09);
	/*wait while cal start is set, cal start is pending, bounded by WURX_OFFSET_CAL_TIMEOUT*/
	bool cal_ok = wurx_wait_reg_bit(CALIB_CTRL,0x01,false,WURX_OFFSET_CAL_TIMEOUT);
	/*wait while cal is running*/
	if(cal_ok)
	{
		cal_ok = wurx_wait_reg_bit(CALIB_STATUS,0x08,false,WURX_OFFSET_CAL_TIMEOUT);
	}

	/*write the saved register values back, needed in any case*/
	wurx_write_reg(BAND_BRANCH_CTRL,bandBranchSelValue);
	wurx_write_reg(CALIB_CTRL,calSelValue);

	/*cal did not finish, report abort and keep the uncalibrated state*/
	if(!cal_ok)
	{
		wurx_state.offsCalStatus=WURX_OFFSET_CAL_ABORTED;
		return wurx_state.offsCalStatus;
	}

	wurx_state.offsCalStatus=WURX_OFFSET_CAL_DONE;

	return wurx_state.offsCalStatus;
}

wurx_lo_exit_event_t wurx_run_lo_cal(void)
{
	/*check if cal is already in process*/
	if(wurx_read_reg(CALIB_STATUS)!=0x00)
	{
		wurx_state.loCalStatus=WURX_LO_CAL_IN_PROCSESS;
		return wurx_state.loCalStatus;
	}
	/*save wakeup register control values which will be changed by lco_cal*/
	uint8_t bandBranchSelValue=wurx_read_reg(BAND_BRANCH_CTRL);
	uint8_t calSelValue=wurx_read_reg(CALIB_CTRL);

	/*set all bands on*/
	wurx_write_reg(BAND_BRANCH_CTRL,WURX_BAND_2G4+WURX_BAND_868+WURX_BAND_433+WURX_BRANCH_STRONG+WURX_BRANCH_WEAK+WURX_BRANCH_MEDIUM);
	/*set cal cfg to lco and run cal*/
	wurx_write_reg(CALIB_CTRL,0x03);
	/*wait while cal start is set, cal start is pending, bounded by WURX_RUN_LO_CAL_TIMEOUT*/
	bool cal_ok = wurx_wait_reg_bit(CALIB_CTRL,0x01,false,WURX_RUN_LO_CAL_TIMEOUT);
	/*wait while cal is running*/
	if(cal_ok)
	{
		cal_ok = wurx_wait_reg_bit(CALIB_STATUS,0x02,false,WURX_RUN_LO_CAL_TIMEOUT);
	}

	/*write the settings back, needed in any case*/
	wurx_write_reg(BAND_BRANCH_CTRL,bandBranchSelValue);
	wurx_write_reg(CALIB_CTRL,calSelValue);

	/*cal did not finish, report abort and keep the uncalibrated state*/
	if(!cal_ok)
	{
		wurx_state.loCalStatus=WURX_LO_CAL_ABORTED;
		return wurx_state.loCalStatus;
	}

	wurx_state.loCalStatus=WURX_LO_CAL_DONE;

	return wurx_state.loCalStatus;
}

bool wurx_set_lo_target(wurx_band_selector_t band, uint16_t target)
{
	if(target==0)
		return false;

	/*calculate target value*/
	uint16_t target_value=0;
	switch (band) {
		case WURX_BAND_433:
			/*calculate target value for 433 MHz band*/
			target_value = (uint16_t)(target*1e6/4/32768);
			/*write target high byte*/
			wurx_write_reg(N_LCO_TARGET_433_8,(target_value>>8)&0xff);
			/*write target low byte*/
			wurx_write_reg(N_LCO_TARGET_433_0,target_value&0xff);
			break;
		case WURX_BAND_868:
			target_value = (uint16_t)(target*1e6/8/32768);
			/*write target high byte*/
			wurx_write_reg(N_LCO_TARGET_868_8,(target_value>>8)&0xff);
			/*write target low byte*/
			wurx_write_reg(N_LCO_TARGET_868_0,target_value&0xff);
			break;
		case WURX_BAND_2G4:
			target_value = (uint16_t)(target*1e6/16/32768);
			/*write target high byte*/
			wurx_write_reg(N_LCO_TARGET_2G4_8,(target_value>>8)&0xff);
			/*write target low byte*/
			wurx_write_reg(N_LCO_TARGET_2G4_0,target_value&0xff);
			break;
		default:
			return false;
			break;
	}
return true;
}

wurx_isr_t wurx_irq_handler(wurx_irq_event_cont_t *store)
{
	/*store already points to the entry that receives the event data*/

	/*read time stamp, high byte first*/
	store->irq_time_stamp = wurx_read_counter();

	/*init fifo variables*/
	store->fifo_count = 0;
	uint8_t data_temp=0;
	uint8_t band_trigger=3;
	/* force slow, disable data receive*/
	/* FO_QUIT ASIC workaround: bit 7 of 0x00/0x01/0x02 is a second function (force LDR).
   The register is documented write-only, but the read-back is valid silicon
   behaviour and is required to keep the configured NFA value. */
	/* check which band is triggered*/
	band_trigger=wurx_read_reg(IDM_BAND);
	/* control band trigger 0=433, 1=868, 2=2G4, 3= forbidden*/
	if(band_trigger != 3)
	{
		/*read the register value given by IDM_BAND*/
		data_temp=wurx_read_reg(band_trigger);
		/*write the value which was read from IDM_BAND|0x80*/
		wurx_write_reg(band_trigger,data_temp|0x80);

	}
	/*read the KORREL_STATE value of the last received code*/
	store->correl_state=wurx_read_reg(KORREL_STATE);
	/*map band data from KORREL_STATE to band_irq_fired variable, bit 7:6 represents band data*/
	store->band_irq_fired = (wurx_band_irq_event_t)((store->correl_state >> 6) & 0x03);
	/*the band information is only valid for 433, 868 and 2G4. KORREL_STATE power on
	  default is 0xC0 which yields 0b11 (forbidden) and would cause out of range FIFO
	  register addresses (e.g. 0x5A instead of 0x57). Fall back to the band reported
	  by IDM_BAND in that case.*/
	if(store->band_irq_fired > WURX_BAND_IRQFIRED_2G4)
	{
		store->band_irq_fired = (wurx_band_irq_event_t)band_trigger;
	}
	/*read last received codeA/B KORREL_VAL value*/
	store->correl_value=wurx_read_reg(KORREL_VAL);
	/*read IRQ_STATUS Event*/
	store->irq_event=wurx_read_reg(IRQ_STATUS);
	/*read IDM reason*/
	store->id_type=wurx_read_reg(IDM_REASON);

	/* check if any data irq events are present, data handler receives irq */
	if(		(store->irq_event&WURX_IDMANDSLOW_IRQ)
			| (store->irq_event&WURX_IDMANDFIFO_IRQ)
			| (store->irq_event&WURX_FIFO_FULL_IRQ)
			| (store->irq_event&WURX_FIFO_OVF_IRQ)
	)
	{
		/*check band_irq_fired for integrity, valid values 0:433 1:868 2:2G4, Invalid 3:no band*/
		if(store->band_irq_fired!=3)
		{
			/* read FiFo byte counter value on triggered band */
			store->fifo_count=wurx_read_reg(FIFO_COUNT_433+store->band_irq_fired);
			/* FiFo Length slow MCU workaround  */
			if(store->irq_event&WURX_FIFO_FULL_IRQ)
			{
				/* compare FIFO_COUNT with FIFO_LENGTH, definition of length value 0->FIFO_LENGTH*/
				data_temp=wurx_read_reg(FIFO_LENGTH);
				/* map length to band*/
				switch (store->band_irq_fired)
				{
				case WURX_BAND_IRQFIRED_433:
					data_temp = ((data_temp&0x03)*8)+16;
					break;
				case WURX_BAND_IRQFIRED_868:
					data_temp = (((data_temp >> 2)&0x03)*8)+16;
					break;
				case WURX_BAND_IRQFIRED_2G4:
					data_temp = (((data_temp >> 4)&0x03)*8)+16;
					break;
				}

				if(data_temp<store->fifo_count)
					store->fifo_count = data_temp;
			}


			/*Readout FIFO Data*/
			for(uint8_t fifo_byte_counter = 0; fifo_byte_counter < 6; fifo_byte_counter++)
			{
				*(store->fifo_data+fifo_byte_counter)=wurx_read_reg((RX_FIFO_0_433+store->band_irq_fired*6)-fifo_byte_counter);
			}
			/*FIFO count will be automatically cleared by ic logic at next interrupt*/
			/*clear FIFO_COUNT register*/
			wurx_write_reg(FIFO_COUNT_433+store->band_irq_fired,0x00);
		}
	}

	/*wait until irq event is cleared, bounded by WURX_IRQ_TIMEOUT counter cycles*/
	uint64_t irq_clear_start = wurx_read_counter();
	wurx_state.irqClearTimeout = false;
	while( (store->irq_event & wurx_read_reg(IRQ_STATUS)))
	{
		/*abort if the event did not clear within the timeout*/
		if( ((wurx_read_counter() - irq_clear_start) & 0xFFFFFFFFFFULL) > (uint64_t)WURX_IRQ_TIMEOUT )
		{
			wurx_state.irqClearTimeout = true;
			return WURX_ISR_CLEAR_TIMEOUT;
		}
		/*clear rtc events, IRQ6 cannot be cleared as long as RTC_STATUS is set*/
		wurx_write_reg(RTC_CLR,0x0f);
		/*repeat the irq clear routine as well as the codeAB irq clearing bug
		 workaround to prevent a deadlock caused by irq glitches*/
		/*clear all irq events*/
		wurx_write_reg(IRQ_CLR,	WURX_IDM_IRQ|
								WURX_FIFO_OVF_IRQ|
								WURX_FIFO_FULL_IRQ|
								WURX_CODEAB_IRQ|
								WURX_IDMANDFIFO_IRQ|
								WURX_IDMANDSLOW_IRQ|
								WURX_RTC_IRQ|
								WURX_RTCC_IRQ);
		/*next part is a codeAB_irq_clear_bug workaround, it is necessary to run
		 the following commands when the WURX_CODEAB_IRQ bit in IRQ_SELECT is enabled,
		 otherwise WURX_CODEAB_IRQ will not be cleared*/
		/*clear KORREL_SV*/
		wurx_write_reg(KORREL_SV_CLEAR,0x01);
//		/*wait a couple of wurx clocks until event is cleared*/
//		wurx_delay_ms(1);
		/*disable korrel vector in val_keeper block*/
		wurx_write_reg(KORREL_SV_CLEAR,0x00);
	}
	/*IRQ_CLR must be set to zero manually after a successful irq event clear*/
	wurx_write_reg(IRQ_CLR,0x00);

	return WURX_ISR_OK;
}

#ifdef WURXIRQSTORE
void wurx_store_irq_event(wurx_irq_event_cont_store_t *store, wurx_irq_event_cont_t event)
{

	if(store->wurx_irqevent_store_counter < (MAXLISTENTRIES))
	{
		store->wurx_irqevent_store[store->wurx_irqevent_store_counter] = event;
		store->wurx_irqevent_store_counter++;
	}
}

wurx_irq_event_cont_store_t* wurx_get_irq_event_array(void)
{
	return &wurx_irq_event_cont_store;
}

wurx_irq_event_cont_t* wurx_get_irq_event_last(void)
{
  if(wurx_irq_event_cont_store.wurx_irqevent_store_counter!=0)
    {
      return &(wurx_irq_event_cont_store.wurx_irqevent_store[wurx_irq_event_cont_store.wurx_irqevent_store_counter-1]);
    }
      return 0;
}

void wurx_clear_irq_count(void)
{
	wurx_irq_event_cont_store.wurx_irqevent_store_counter = 0;
}

uint8_t wurx_get_irq_event_counter(void)
{
	return wurx_irq_event_cont_store.wurx_irqevent_store_counter;
}
#endif

bool wurx_init(wurx_config_t config)
{
	/*read WakeUp chip version*/
	if(WURXICVERSION != wurx_read_reg(VERSION))
	{
		/*something is wrong, spi communication was not successful, the read WURX IC VERSION is wrong*/
	  wurx_state.icIdGood = false;
	  wurx_state.receiverGood = false;
		return wurx_state.receiverGood;
	}
	else
	{
		wurx_state.icIdGood = true;
	}
  	/*set main clk feed to given parameter by config*/
  	wurx_init_xtal(config.wurx_clk_mode);

	/*set register values, datasheet Figure 9 power up routine*/
	wurx_write_reg(D_CORNER_CTRL,0x02);
	wurx_write_reg(LC_TG_ENA,0x00);
	wurx_write_reg(COMP_THRESH_W,0x0a);

	/*set data rate and run lo cal*/
	/*run lo cal on band 2g4*/
	if(config.band_selector&WURX_BAND_2G4)
	{
		/*set preamble and data rates on 2g4*/
		wurx_set_data_rate(config.pream_data_rate_2G4,config.data_rate_2G4,WURX_BAND_2G4);
		/*set target on corresponding band*/
		wurx_set_lo_target(WURX_BAND_2G4,config.lo_target_2g4);
	}
	/*run lo cal on band 868*/
	if(config.band_selector&WURX_BAND_868)
	{
		/*set preamble and data rates on 868*/
		wurx_set_data_rate(config.pream_data_rate_868,config.data_rate_868,WURX_BAND_868);
		/*set target on corresponding band*/
		wurx_set_lo_target(WURX_BAND_868,config.lo_target_868);
	}
	/*run lo cal on band 433*/
	if(config.band_selector&WURX_BAND_433)
	{
		/*set preamble and data rates on 433*/
		wurx_set_data_rate(config.pream_data_rate_433,config.data_rate_433,WURX_BAND_433);
		/*set target on corresponding band*/
		wurx_set_lo_target(WURX_BAND_433,config.lo_target_433);
	}
	/*start lo cal*/
	wurx_run_lo_cal();
	/*start offset cal*/
	wurx_offset_cal();
	/*set band and branch*/
	wurx_set_band_branch(config.band_selector,config.branch_selector);
	/*set fdd*/
	wurx_write_reg(FDD_ENABLE,config.fdd_selector);
	/*set idm*/
	wurx_write_reg(IDM_ENABLE,config.idm_selector);
	/*set irq source*/
	wurx_write_reg(IRQ_SELECT,config.irq_source);
	/*set id high byte*/
	wurx_write_reg(ID_HI,config.id>>8);
	/*set id low byte*/
	wurx_write_reg(ID_LO,config.id&0xff);
	/*idm reason*/
	wurx_write_reg(IDM_CTRL,config.id_reason);
	/*set dout mux*/
	wurx_write_reg(MUX_D_OUT_SEL,config.dout_mux);

	/*check all IC state flags and set overall status flag*/
	if(  (wurx_state.xtalGood==true)
	    &(wurx_state.loCalStatus==WURX_LO_CAL_DONE)
	    &(wurx_state.offsCalStatus==WURX_OFFSET_CAL_DONE)
	){
	    wurx_state.receiverGood = true;
	}
	else
	  {
	    wurx_state.receiverGood = false;
	  }
	return wurx_state.receiverGood;
}

/**
 * @brief reset FH101RF registers to default values
 * @note weak default implementation is intentionally empty. Override it in the
 *       platform layer if a real register reset is required.
 */
void wurx_reset_register(void)
{
	/*empty weak default, override in platform layer*/
}

void wurx_set_data_rate(wurx_drate_selector_t pream, wurx_drate_selector_t data, wurx_band_selector_t band)
{
	switch (band) {
		case WURX_BAND_433:
			/*set preamble data rate*/
			wurx_write_reg(NFA433_SLOW,pream);
			/*set payload data rate*/
			wurx_write_reg(NFA433_FAST,data);
			break;
		case WURX_BAND_868:
			/*set preamble data rate*/
			wurx_write_reg(NFA868_SLOW,pream);
			/*set payload data rate*/
			wurx_write_reg(NFA868_FAST,data);
			break;
		case WURX_BAND_2G4:
			/*set preamble data rate*/
			wurx_write_reg(NFA2G4_SLOW,pream);
			/*set payload data rate*/
			wurx_write_reg(NFA2G4_FAST,data);
			break;
		case WURX_BAND_ALL_OFF:
		break;
	}
}

void wurx_set_band_branch(uint8_t band, wurx_branch_selector_t branch)
{

	wurx_write_reg(BAND_BRANCH_CTRL,band+branch);
}

void wurx_init_xtal(wurx_clk_mode_t mode)
{
  /*temporary storage for the xtal control register value*/
  uint8_t xtalCtrlValue = 0;

  switch (mode) {
    case WURX_EXT_CLK:
      /*read back the current register value first, bits 2:0 (INT<1:0>) are
        reserved and must not be changed, so the bypass bit is added to the
        existing setting*/
      xtalCtrlValue = wurx_read_reg(XTAL_OSC_BYP);
      xtalCtrlValue |= (1<<3);
      wurx_write_reg(XTAL_OSC_BYP,xtalCtrlValue);
      wurx_write_reg(XTAL_OSC_ENA,0);
      break;
    case WURX_XTAL_CLK:
      /*crystal on XI and XO, restore the reset default values of both registers
        so a previous external clock setup cannot leave the bypass bit set*/
      xtalCtrlValue = WURX_XTAL_OSC_BYP_DEFAULT;
      wurx_write_reg(XTAL_OSC_BYP,xtalCtrlValue);
      wurx_write_reg(XTAL_OSC_ENA,WURX_XTAL_OSC_ENA_DEFAULT);
      break;
    default:
      break;
  }

/*wait until the crystal oscillator reports a stable clock, bounded by WURX_INIT_XTAL_TIMEOUT*/
	wurx_state.xtalGood = wurx_wait_reg_bit(XTAL_GOOD,0x01,true,WURX_INIT_XTAL_TIMEOUT);
}

void wurx_set_irq_source(wurx_irq_event_t source)
{
  wurx_write_reg(IRQ_SELECT,source);
}

void wurx_set_rtc_alarm(wurx_timer_types_t timer, uint64_t alarm)
{
  /*clear rtc status*/
  wurx_write_reg(RTC_CLR,0x0f);
  /*enable selected rtc*/
  wurx_write_reg(RTC_SELECT,timer);

  /*set rtc alarm*/
    switch (timer) {
      case WURX_CYCL:
        ; //todo
        break;
      case WURX_RTCSH0:
        wurx_write_reg(RTCSH0_THRESH_8,(alarm >> 8));
        wurx_write_reg(RTCSH0_THRESH_0,(alarm&0xff));
        break;
      case WURX_RTCSH1:
        wurx_write_reg(RTCSH1_THRESH_8,(alarm >> 8));
        wurx_write_reg(RTCSH1_THRESH_0,(alarm&0xff));
        break;
      case WURX_RTCLG0:
        wurx_write_reg(RTCLG0_THRESH_32,(alarm >> 32));
        wurx_write_reg(RTCLG0_THRESH_24,(alarm >> 24));
        wurx_write_reg(RTCLG0_THRESH_16,(alarm >> 16));
        wurx_write_reg(RTCLG0_THRESH_8,(alarm >> 8));
        wurx_write_reg(RTCLG0_THRESH_0,(alarm&0xff));
        break;
      case WURX_RTCLG1:
        wurx_write_reg(RTCLG1_THRESH_32,(alarm >> 32));
        wurx_write_reg(RTCLG1_THRESH_24,(alarm >> 24));
        wurx_write_reg(RTCLG1_THRESH_16,(alarm >> 16));
        wurx_write_reg(RTCLG1_THRESH_8,(alarm >> 8));
        wurx_write_reg(RTCLG1_THRESH_0,(alarm&0xff));
        break;
      default:
        break;
    }
}
