/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 14, 2018
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include "ble/cs_Nordic.h"
#include "events/cs_EventListener.h"
#include "util/cs_Utils.h"


#define CMD_ADV_NUM_SERVICES_16BIT 4 // There are 4 16 bit service UUIDs in a command advertisement.

struct __attribute__((__packed__)) CommandAdvertisementHeader {
	uint8_t sequence0 : 2;
	uint8_t protocol : 3;
	uint8_t sphereId : 8;
	uint8_t accessLevel : 3;
//
//	uint8_t sequence1 : 2;
//	uint16_t reserved : 10;
//	uint16_t payload1 : 4;
//
//	uint8_t sequence2 : 2;
//	uint16_t payload2 : 14;
//
//	uint8_t sequence3 : 2;
//	uint16_t payload3 : 14;
};

struct __attribute__((__packed__)) PayloadRC5 {
	uint8_t locationId : 6;
	uint8_t profileId : 3;
	int8_t rssiOffset : 4;
	uint8_t flags : 3;
	// 16bit reserved
};

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


