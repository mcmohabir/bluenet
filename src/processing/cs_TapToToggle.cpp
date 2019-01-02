/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 2, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "processing/cs_TapToToggle.h"
#include "events/cs_EventDispatcher.h"
#include "drivers/cs_RTC.h"
#include "drivers/cs_Serial.h"

TapToToggle::TapToToggle() {
//	timeoutTicks = RTC::msToTicks(3000);
	EventDispatcher::getInstance().addListener(this);
}

void TapToToggle::handleBackgroundAdvertisement(evt_adv_background_t* adv) {
	if (adv->rssi < rssiThreshold) {
		return;
	}
//	uint8_t index = -1; // Index to use
//	bool useOldAverage = false;
//	uint32_t oldestTimestamp = -1;
//	uint32_t currentTime = RTC::getCount();
//	for (uint8_t i=0; i<T2T_LIST_COUNT; ++i) {
//		if (memcmp(list[i].address, adv->macAddress, BLE_GAP_ADDR_LEN) == 0) {
//			index = i;
//			if (RTC::difference(currentTime, list[i].lastTimestamp) < timeoutTicks) {
//				useOldAverage = true;
//			}
//			break;
//		}
//		if (list[i].lastTimestamp < oldestTimestamp) {
//			oldestTimestamp = list[i].lastTimestamp;
//			index = i;
//		}
//	}
//
//	if (!useOldAverage) {
//		list[index].avgRssi = adv->rssi;
//		memcpy(list[index].address, adv->macAddress, BLE_GAP_ADDR_LEN);
//	}
//	list[index].lastTimestamp = currentTime;
//	list[index].

	// Use index of entry with matching address, or else with lowest score.
	uint8_t index = -1; // Index to use
	bool foundAddress = false;
	uint8_t lowestScore = 255;
	for (uint8_t i=0; i<T2T_LIST_COUNT; ++i) {
		if (memcmp(list[i].address, adv->macAddress, BLE_GAP_ADDR_LEN) == 0) {
			index = i;
			foundAddress = true;
			break;
		}
		if (list[i].score < lowestScore) {
			lowestScore = list[i].score;
			index = i;
		}
	}
	if (!foundAddress) {
		memcpy(list[index].address, adv->macAddress, BLE_GAP_ADDR_LEN);
		list[index].score = 0;
	}

	uint8_t prevScore = list[index].score;
	list[index].score += 2;
	if (list[index].score > scoreMax) {
		list[index].score = scoreMax;
	}

	LOGd("rssi=%u ind=%u prevScore=%u score=%u", adv->rssi, index, prevScore, list[index].score);
	if (prevScore <= scoreThreshold && list[index].score > scoreThreshold) {
		LOGi("TRIGGER");
	}
}

void TapToToggle::tick() {
	for (uint8_t i=0; i<T2T_LIST_COUNT; ++i) {
		if (list[i].score > 0) {
			list[i].score--;
		}
	}
}

void TapToToggle::handleEvent(uint16_t evt, void* data, uint16_t length) {
	switch(evt) {
	case EVT_TICK_500_MS: {
		tick();
		break;
	}
	case EVT_ADV_BACKGROUND_PARSED: {
		if (length != sizeof(evt_adv_background_t)) {
			return;
		}
		handleBackgroundAdvertisement((evt_adv_background_t*) data);
		break;
	}
	}
}

