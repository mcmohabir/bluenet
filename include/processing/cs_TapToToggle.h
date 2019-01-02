/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 2, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include "events/cs_EventListener.h"
#include "ble/cs_Nordic.h"

//struct __attribute__((__packed__)) t2t_entry_t {
//	uint32_t lastTimestamp;
//	uint8_t address[BLE_GAP_ADDR_LEN];
//	int16_t avgRssi; // rssi multiplied by 256
//};

struct __attribute__((__packed__)) t2t_entry_t {
	uint8_t address[BLE_GAP_ADDR_LEN];
	uint8_t score; // Score that decreases every tick, and increases when rssi is above threshold.
};

#define T2T_LIST_COUNT 3

class TapToToggle : public EventListener {
public:
	static TapToToggle& getInstance() {
		static TapToToggle instance;
		return instance;
	}
	void handleEvent(uint16_t evt, void* data, uint16_t length);

private:
	t2t_entry_t list[T2T_LIST_COUNT];
//	uint32_t timeoutTicks;
	int8_t rssiThreshold = -50;
	uint8_t scoreIncrement = 2; // Score is increased with this value when rssi is above rssi threshold.
	uint8_t scoreThreshold = 2; // Threshold above which the toggle is triggered.
	uint8_t scoreMax = 4;       // Score can't be higher than this value.
	TapToToggle();
	void handleBackgroundAdvertisement(evt_adv_background_t* adv);
	void tick();
};
