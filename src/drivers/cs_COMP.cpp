/*
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jan 31, 2017
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include "drivers/cs_COMP.h"

#include "util/cs_BleError.h"
#include "drivers/cs_Serial.h"

//#include <app_util_platform.h>

extern "C" void comp_callback(nrf_comp_event_t event);

extern "C" void comp_work_around() { *(volatile uint32_t *)0x40013540 = (*(volatile uint32_t *)0x10000324 & 0x00001F00) >> 8; }

COMP::COMP() {

}

void COMP::init() {
	LOGd("init");
//	applyWorkarounds();
	comp_work_around();
	config();

//	nrf_comp_task_trigger(NRF_COMP_TASK_STOP);
//	nrf_comp_enable();
//
//	nrf_comp_event_clear(NRF_COMP_EVENT_READY);
//	nrf_comp_event_clear(NRF_COMP_EVENT_DOWN);
//	nrf_comp_event_clear(NRF_COMP_EVENT_UP);
//	nrf_comp_event_clear(NRF_COMP_EVENT_CROSS);
//
//	nrf_comp_ref_set(NRF_COMP_REF_Int2V4);
//	nrf_comp_th_t threshold;
//	threshold.th_down = 30;
//	threshold.th_up = 34;
//	nrf_comp_th_set(threshold);
//
//	nrf_comp_main_mode_set(NRF_COMP_MAIN_MODE_SE);
//	nrf_comp_speed_mode_set(NRF_COMP_SP_MODE_High);
//	nrf_comp_hysteresis_set(NRF_COMP_HYST_NoHyst);
//	nrf_comp_isource_set(NRF_COMP_ISOURCE_Off);
//	nrf_comp_shorts_disable(COMP_SHORTS_CROSS_STOP_Msk | COMP_SHORTS_UP_STOP_Msk | COMP_SHORTS_DOWN_STOP_Msk);
//	nrf_comp_int_disable(COMP_INTENCLR_CROSS_Msk | COMP_INTENCLR_UP_Msk | COMP_INTENCLR_DOWN_Msk | COMP_INTENCLR_READY_Msk);
//	nrf_comp_input_select(NRF_COMP_INPUT_3);
//    NVIC_SetPriority(COMP_LPCOMP_IRQn, APP_IRQ_PRIORITY_LOW);
//    NVIC_ClearPendingIRQ(COMP_LPCOMP_IRQn);
//    NVIC_EnableIRQ(COMP_LPCOMP_IRQn);

}

void COMP::config() {
//	nrf_comp_th_t threshold = {
//			.th_down = VOLTAGE_THRESHOLD_TO_INT(0.5, 2.4),
//			.th_up = VOLTAGE_THRESHOLD_TO_INT(1.6, 2.4)
//	};
	nrf_comp_th_t threshold;
	threshold.th_down = 30;
	threshold.th_up = 34;


//	nrf_drv_comp_config_t config = {
//			.reference = NRF_COMP_REF_Int2V4, // Reference of 2.4V
////			.ext_ref = // Not used, as we use an internal reference
//			.main_mode = NRF_COMP_MAIN_MODE_SE, // Single ended, not differential
//			.threshold = threshold,
//			.speed_mode = NRF_COMP_SP_MODE_Low, // Delay of 0.6us
////			.hyst = NRF_COMP_HYST_NoHyst, // Not used in single ended mode
//			.isource = NRF_COMP_ISOURCE_Off, // Should be off in our case and due to PAN 84
//			.input = NRF_COMP_INPUT_3, // AIN3
//			.interrupt_priority = APP_IRQ_PRIORITY_LOW
//	};
//	nrf_drv_comp_config_t config = {
//			.reference = NRF_COMP_REF_Int2V4, // Reference of 2.4V
//			.main_mode = NRF_COMP_MAIN_MODE_SE, // Single ended, not differential
//			.threshold = threshold,
//			.speed_mode = NRF_COMP_SP_MODE_Low, // Delay of 0.6us
//			.isource = NRF_COMP_ISOURCE_Off, // Should be off in our case and due to PAN 84
//			.input = NRF_COMP_INPUT_3, // AIN3
//			.interrupt_priority = APP_IRQ_PRIORITY_LOW
//	};
	nrf_drv_comp_config_t config;
//	config.reference = NRF_COMP_REF_Int2V4; // Reference of 2.4V
	config.reference = NRF_COMP_REF_Int1V8;
	config.main_mode = NRF_COMP_MAIN_MODE_SE; // Single ended, not differential
	config.threshold = threshold;
//	config.speed_mode = NRF_COMP_SP_MODE_Low; // Delay of 0.6us
	config.speed_mode = NRF_COMP_SP_MODE_High; // Delay of 0.1us
	config.hyst = NRF_COMP_HYST_NoHyst; // Not used in single ended mode
	config.isource = NRF_COMP_ISOURCE_Off; // Should be off in our case and due to PAN 84
	config.input = NRF_COMP_INPUT_3; // AIN3 gpio5
	config.interrupt_priority = APP_IRQ_PRIORITY_LOW;

	ret_code_t err_code = nrf_drv_comp_init(&config, comp_callback);
	APP_ERROR_CHECK(err_code);
}



void COMP::start() {
	LOGd("start");
	nrf_drv_comp_start(NRF_DRV_COMP_EVT_EN_UP_MASK | NRF_DRV_COMP_EVT_EN_DOWN_MASK, 0);
//	nrf_comp_int_enable(COMP_INTENSET_UP_Msk | COMP_INTENSET_DOWN_Msk);
//	nrf_comp_task_trigger(NRF_COMP_TASK_START);
}

uint32_t COMP::sample() {
	uint32_t sample = nrf_drv_comp_sample();
//	nrf_comp_task_trigger(NRF_COMP_TASK_SAMPLE);
//	uint32_t sample = nrf_comp_result_get();
	return sample;
}

void COMP::handleEvent(nrf_comp_event_t event) {
	LOGd("event");
	switch (event) {
	case NRF_COMP_EVENT_READY:
		LOGd("ready");
		break;
	case NRF_COMP_EVENT_DOWN:
		LOGd("down");
		break;
	case NRF_COMP_EVENT_UP:
		LOGd("up");
		break;
	case NRF_COMP_EVENT_CROSS:
		LOGd("cross");
		break;
	}
}

extern "C" void comp_callback(nrf_comp_event_t event) {
	COMP::getInstance().handleEvent(event);
}

//extern "C" void COMP_LPCOMP_IRQHandler(void) {
//	COMP::getInstance().handleEvent(NRF_COMP_EVENT_READY);
//}
