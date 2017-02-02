/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 4 Nov., 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

#include "drivers/cs_LPComp.h"

#include <ble/cs_Nordic.h>

//#include "nrf.h"
//#include "nrf_delay.h"
//#include "nrf_gpio.h"
//
//
//#if(NRF51_USE_SOFTDEVICE == 1)
//#include "nrf_sdm.h"
//#endif
//
//#include "common/cs_Boards.h"
#include "drivers/cs_Serial.h"
#include "drivers/cs_PWM.h"
#include "util/cs_BleError.h"
//
//#include "cs_nRF51822.h"

#include <app_util_platform.h>

#include "nrf_delay.h"
#include <cfg/cs_Strings.h>

#include <nrf_lpcomp.h>

//#define PRINT_LPCOMP_VERBOSE

LPComp::LPComp() {

}

LPComp::~LPComp() {
	stop();
}

void LPComp::init() {
	config(3, 3, LPC_UP);
}

/**
 * The init function is called once before operating the LPComp. Call it after you start the SoftDevice. Check
 * the section 32 "Low Power Comparator (LPCOMP)" in the nRF51 Series Reference Manual.
 *   - select the input pin (analog in)
 *   - use the internal VDD reference
 * The level can be set to from 0 to 6 , making it compare to 1/8 to 7/8 of the supply voltage.
 */
uint32_t LPComp::config(uint8_t pin, uint8_t level, Event_t event) {
//#if (NORDIC_SDK_VERSION < 11)
#if(NRF51_USE_SOFTDEVICE == 1)
	LOGd("Run LPComp with SoftDevice");
#else
	LOGd("Run LPComp without SoftDevice!!!");
#endif

	NRF_LPCOMP->ENABLE = LPCOMP_ENABLE_ENABLE_Disabled << LPCOMP_ENABLE_ENABLE_Pos;

	nrf_lpcomp_config_t config;
	config.reference = NRF_LPCOMP_REF_SUPPLY_4_8;
	config.detection = NRF_LPCOMP_DETECT_DOWN;
	nrf_lpcomp_configure(&config);

	nrf_delay_us(500);

// 	nrf_lpcomp_input_select(NRF_LPCOMP_INPUT_7);
	NRF_LPCOMP->PSEL   = (LPCOMP_PSEL_PSEL_AnalogInput2 << LPCOMP_PSEL_PSEL_Pos);

//	nrf_lpcomp_int_enable(LPCOMP_INTENSET_CROSS_Msk);
	NRF_LPCOMP->INTENSET = LPCOMP_INTENSET_CROSS_Msk;

	nrf_delay_us(500);

	uint32_t inten = NRF_LPCOMP->INTENSET;
	LOGd("inten set to %u, now is %u", LPCOMP_INTENSET_CROSS_Msk, inten);
//	nrf_lpcomp_shorts_enable(NRF_LPCOMP_SHORT_READY_SAMPLE_MASK);

	//! Enable LPComp interrupt

	// Note! These are coming from nrf51.h?
	uint32_t err_code;
	err_code = sd_nvic_SetPriority(LPCOMP_IRQn, APP_IRQ_PRIORITY_HIGH);
	APP_ERROR_CHECK(err_code);
	LOGd("PWM1_IRQn=%i", PWM1_IRQn); //should be 33
	err_code = sd_nvic_ClearPendingIRQ(LPCOMP_IRQn);
	APP_ERROR_CHECK(err_code);
	err_code = sd_nvic_EnableIRQ(LPCOMP_IRQn);
	APP_ERROR_CHECK(err_code);

//	NRF_LPCOMP->POWER = LPCOMP_POWER_POWER_Enabled << LPCOMP_POWER_POWER_Pos;
	return 0;
}

/**
 * Stop the LP comparator.
 */
void LPComp::stop() {
	nrf_lpcomp_disable();
	nrf_lpcomp_task_trigger(NRF_LPCOMP_TASK_STOP);
}

/**
 * Start the LP comparator.
 */
void LPComp::start() {
	LOGd(FMT_START, "LPComp");
	nrf_lpcomp_enable();
	nrf_delay_us(500);
//	nrf_lpcomp_task_trigger(NRF_LPCOMP_TASK_START);
	NRF_LPCOMP->TASKS_START = 1;
	nrf_delay_us(500);
    NRF_LPCOMP->EVENTS_READY = 0;
    NRF_LPCOMP->EVENTS_DOWN  = 0;
    NRF_LPCOMP->EVENTS_UP    = 0;
    NRF_LPCOMP->EVENTS_CROSS = 0;

 	bool enabled = nrf_lpcomp_int_enable_check(LPCOMP_INTENSET_CROSS_Msk);
 	LOGd("enabled=%u", enabled);
}

uint32_t LPComp::sample() {
//	nrf_lpcomp_task_trigger(NRF_LPCOMP_TASK_SAMPLE);
	NRF_LPCOMP->TASKS_SAMPLE = 1;
 	return nrf_lpcomp_result_get();
}


void LPComp::interrupt() {
	LOGw("interrupt!");
//	PWM::getInstance().setValue(0, 0);
}


/*
 * The interrupt handler for the LCOMP events.
 * name defined in nRF51822.c
 */
extern "C" void WUCOMP_COMP_IRQHandler(void) {
	APP_ERROR_CHECK(0xFFFFFFFF); //! error
	LPComp::getInstance().interrupt();
	nrf_lpcomp_event_clear(NRF_LPCOMP_EVENT_READY);
	nrf_lpcomp_event_clear(NRF_LPCOMP_EVENT_DOWN);
	nrf_lpcomp_event_clear(NRF_LPCOMP_EVENT_UP);
	nrf_lpcomp_event_clear(NRF_LPCOMP_EVENT_CROSS);

}


