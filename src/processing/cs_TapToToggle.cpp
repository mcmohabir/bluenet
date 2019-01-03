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
#include "processing/cs_CommandHandler.h"
#include "util/cs_Utils.h"
#include "storage/cs_Settings.h"

TapToToggle::TapToToggle() {
	Settings::getInstance().get(CONFIG_T2T_SCORE_INCREMENT, &scoreIncrement, false);
	Settings::getInstance().get(CONFIG_T2T_SCORE_THRESHOLD, &scoreThreshold, false);
	Settings::getInstance().get(CONFIG_T2T_SCORE_MAX, &scoreMax, false);
	EventDispatcher::getInstance().addListener(this);
}

void TapToToggle::handleBackgroundAdvertisement(evt_adv_background_t* adv) {
	if (adv->rssi < rssiThreshold) {
		return;
	}
	if (adv->dataSize != sizeof(evt_adv_background_payload_t)) {
		return;
	}
//	evt_adv_background_payload_t* payload = (evt_adv_background_payload_t*)(adv->data);
//	if (!BLEutil::isBitSet(payload->flags, 0)) {
//		return;
//	}

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
	list[index].score += scoreIncrement;
	if (list[index].score > scoreMax) {
		list[index].score = scoreMax;
	}

	LOGd("rssi=%i ind=%u prevScore=%u score=%u", adv->rssi, index, prevScore, list[index].score);
	if (prevScore <= scoreThreshold && list[index].score > scoreThreshold) {
		LOGw("-------");
		LOGw("TRIGGER");
		LOGw("-------");
		EventDispatcher::getInstance().dispatch(EVT_POWER_TOGGLE);
	}
}

void TapToToggle::tick() {
	for (uint8_t i=0; i<T2T_LIST_COUNT; ++i) {
		if (list[i].score > 0) {
			list[i].score--;
		}
	}
	LOGd("scores=%u %u %u", list[0].score, list[1].score, list[2].score)
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

