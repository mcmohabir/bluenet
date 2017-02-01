/*
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 6 Nov., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

#include <cstdint>


extern "C" {
#include <nrf_drv_lpcomp.h>
}

/** Compare voltage level (hardware peripheral)
 */
class LPComp {
public:
	//! Use static variant of singleton, no dynamic memory allocation
	static LPComp& getInstance() {
		static LPComp instance;
		return instance;
	}

	void init();
	void start();
	void stop();

	//! function to be called from interrupt, do not do much there!
	void handleEvent(nrf_lpcomp_event_t event);

private:
	LPComp();
	LPComp(LPComp const&); //! singleton, deny implementation
	void operator=(LPComp const &); //! singleton, deny implementation
	~LPComp();

};
