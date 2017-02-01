/*
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jan 31, 2017
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#pragma once

#include <cstdint>

//#include <nrf_comp.h>

extern "C" {
#include <nrf_drv_comp.h>
}

class COMP {
public:
	//! Use static variant of singleton, no dynamic memory allocation
	static COMP& getInstance() {
		static COMP instance;
		return instance;
	}

	void init();
	void start();

	uint32_t sample();

	void handleEvent(nrf_comp_event_t event);

private:
	COMP();

	//! This class is singleton, deny implementation
	COMP(COMP const&);
	//! This class is singleton, deny implementation
	void operator=(COMP const &);

	void config();

	void applyWorkarounds() {
		// PAN 12 COMP: Reference ladder is not correctly calibrated
		*(volatile uint32_t *)0x40013540 = (*(volatile uint32_t *)0x10000324 & 0x00001F00) >> 8;
	}

};
