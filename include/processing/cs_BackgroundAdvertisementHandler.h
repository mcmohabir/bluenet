/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 19, 2018
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include "ble/cs_Nordic.h"
#include "events/cs_EventListener.h"
#include "util/cs_Utils.h"

// See https://www.bluetooth.com/specifications/assigned-numbers/company-identifiers
#define COMPANY_ID_APPLE 0x004C
#define BACKGROUND_SERVICES_MASK_TYPE 0x01
#define BACKGROUND_SERVICES_MASK_HEADER_LEN 3
#define BACKGROUND_SERVICES_MASK_LEN 16

//// 42 bits, so it fits 3 times in 128 bits.
//struct __attribute__((__packed__)) BackgroundAdvertisement {
//	uint8_t protocol : 2;
//	uint8_t sphereId : 8;
//	uint16_t encryptedData[2];
//};

//struct __attribute__((__packed__)) BackgroundAdvertisementPayload {
//	uint8_t locationId : 6;
//	uint8_t profileId : 3;
//	int8_t rssiOffset : 4;
//	uint8_t flags : 3;
//};

class BackgroundAdvertisementHandler : public EventListener {
public:
	static BackgroundAdvertisementHandler& getInstance() {
		static BackgroundAdvertisementHandler instance;
		return instance;
	}
	void handleEvent(uint16_t evt, void* data, uint16_t length);


private:
	BackgroundAdvertisementHandler();

	void parseAdvertisement(ble_gap_evt_adv_report_t* advReport);
	void handleBackgroundAdvertisement(evt_adv_background_t* backgroundAdvertisement);
	void adjustRssi(evt_adv_background_t* backgroundAdvertisement, const evt_adv_background_payload_t& payload);
};


