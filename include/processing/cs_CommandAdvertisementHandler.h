/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 14, 2018
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include "ble/cs_Nordic.h"
#include "events/cs_EventListener.h"

class CommandAdvertisementHandler : public EventListener {
public:
	static CommandAdvertisementHandler& getInstance() {
		static CommandAdvertisementHandler instance;
		return instance;
	}
	void handleEvent(uint16_t evt, void* data, uint16_t length);


private:
	CommandAdvertisementHandler();

	void parseAdvertisement(ble_gap_evt_adv_report_t* advReport);
};


