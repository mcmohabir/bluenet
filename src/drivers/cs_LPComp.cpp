/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 4 Nov., 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

#include "drivers/cs_LPComp.h"

#include "drivers/cs_Serial.h"
#include "util/cs_BleError.h"

#include <app_util_platform.h>

//#define PRINT_LPCOMP_VERBOSE


extern "C" void lpcomp_callback(nrf_lpcomp_event_t event);

LPComp::LPComp() {
}

LPComp::~LPComp() {
	stop();
}

void LPComp::init() {
	LOGd("init");
	nrf_drv_lpcomp_config_t config;
	config.hal.reference = NRF_LPCOMP_REF_SUPPLY_4_8;
	config.hal.detection = NRF_LPCOMP_DETECT_CROSS;
	config.input = NRF_LPCOMP_INPUT_3;
	config.interrupt_priority = APP_IRQ_PRIORITY_LOW;

	uint32_t err_code = nrf_drv_lpcomp_init(&config, lpcomp_callback);
	APP_ERROR_CHECK(err_code);
}


/**
 * Stop the LP comparator.
 */
void LPComp::stop() {
	nrf_drv_lpcomp_disable();
}

/**
 * Start the LP comparator.
 */
void LPComp::start() {
	LOGd("start");
	nrf_drv_lpcomp_enable();
}


void LPComp::handleEvent(nrf_lpcomp_event_t event) {
	LOGw("interrupt! %u", event);
}


extern "C" void lpcomp_callback(nrf_lpcomp_event_t event) {
	LPComp::getInstance().handleEvent(event);
}


