/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 19, 2018
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */



#include "processing/cs_BackgroundAdvertisementHandler.h"
#include "drivers/cs_Serial.h"
#include "util/cs_Utils.h"
#include "events/cs_EventDispatcher.h"
#include "ble/cs_Nordic.h"
#include "events/cs_EventTypes.h"
#include "processing/cs_EncryptionHandler.h"
#include "processing/cs_CommandHandler.h"
#include "storage/cs_State.h"

//#define BACKGROUND_ADV_VERBOSE

BackgroundAdvertisementHandler::BackgroundAdvertisementHandler() {
	EventDispatcher::getInstance().addListener(this);
}

void BackgroundAdvertisementHandler::parseAdvertisement(ble_gap_evt_adv_report_t* advReport) {
	uint32_t errCode;
	data_t manufacturerData;
	errCode = BLEutil::findAdvType(BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA, advReport->data, advReport->dlen, &manufacturerData);
	if (errCode != ERR_SUCCESS) {
		return;
	}
	if (manufacturerData.len != BACKGROUND_SERVICES_MASK_HEADER_LEN + BACKGROUND_SERVICES_MASK_LEN) {
		return;
	}

	uint16_t companyId = *((uint16_t*)(manufacturerData.data));
	if (companyId != COMPANY_ID_APPLE) {
		return;
	}
	uint8_t appleAdvType = manufacturerData.data[2];
	if (appleAdvType != BACKGROUND_SERVICES_MASK_TYPE) {
		return;
	}

	uint8_t* servicesMask; // This is a mask of 128 bits.
	servicesMask = manufacturerData.data + BACKGROUND_SERVICES_MASK_HEADER_LEN;

#ifdef BACKGROUND_ADV_VERBOSE
	logSerial(SERIAL_DEBUG, "servicesMask: ");
	BLEutil::printArray(servicesMask, BACKGROUND_SERVICES_MASK_LEN);
#endif

	// Put the data into large integers, so we can perform shift operations.
//	uint64_t left = *((uint64_t*)servicesMask);
//	uint64_t right = *((uint64_t*)(servicesMask + 8));
	uint64_t left =
			((uint64_t)servicesMask[0] << 56) +
			((uint64_t)servicesMask[1] << 48) +
			((uint64_t)servicesMask[2] << 40) +
			((uint64_t)servicesMask[3] << 32) +
			((uint64_t)servicesMask[4] << 24) +
			((uint64_t)servicesMask[5] << 16) +
			((uint64_t)servicesMask[6] << 8 ) +
			((uint64_t)servicesMask[7] << 0 );
	uint64_t right =
				((uint64_t)servicesMask[0+8] << 56) +
				((uint64_t)servicesMask[1+8] << 48) +
				((uint64_t)servicesMask[2+8] << 40) +
				((uint64_t)servicesMask[3+8] << 32) +
				((uint64_t)servicesMask[4+8] << 24) +
				((uint64_t)servicesMask[5+8] << 16) +
				((uint64_t)servicesMask[6+8] << 8 ) +
				((uint64_t)servicesMask[7+8] << 0 );
#ifdef BACKGROUND_ADV_VERBOSE
	LOGd("left=%llx right=%llx", left, right);
#endif


	// Divide the data into 3 parts, and do a bitwise majority vote, to correct for errors.
	uint64_t part1 = (left >> (64-42))  & 0x03FFFFFFFFFF; // First 42 bits from left.
	uint64_t part2 = ((left & 0x3FFFFF) << 20) | ((right >> (64-20)) & 0x0FFFFF); // Last 64-42=22 bits from left, and first 42−(64−42)=20 bits from right.
	uint64_t part3 = (right >> 2) & 0x03FFFFFFFFFF; // Bits 21-62 from right.
	uint64_t result = ((part1 & part2) | (part2 & part3) | (part1 & part3)); // The majority vote
#ifdef BACKGROUND_ADV_VERBOSE
	LOGd("part1=%llx part2=%llx part3=%llx result=%llx", part1, part2, part3, result);
#endif

	// Parse the resulting data.
	evt_adv_background_t backgroundAdvertisement;
	uint16_t encryptedPayload[2];
	backgroundAdvertisement.protocol = (result >> (42-2)) & 0x03;
	backgroundAdvertisement.sphereId = (result >> (42-2-8)) & 0xFF;
	encryptedPayload[0] = (result >> (42-2-8-16)) & 0xFFFF;
	encryptedPayload[1] = (result >> (42-2-8-32)) & 0xFFFF;
	backgroundAdvertisement.macAddress = advReport->peer_addr.addr;
	backgroundAdvertisement.rssi = advReport->rssi;

#ifdef BACKGROUND_ADV_VERBOSE
	LOGd("encrypted=[%u %u]", encryptedPayload[0], encryptedPayload[1]);
#endif
	// TODO: can decrypt to same buffer?
	uint16_t decryptedPayload[2];
	EncryptionHandler::getInstance().RC5Decrypt(encryptedPayload, sizeof(encryptedPayload), decryptedPayload, sizeof(decryptedPayload));
#ifdef BACKGROUND_ADV_VERBOSE
	LOGd("decrypted=[%u %u]", decryptedPayload[0], decryptedPayload[1]);
#endif

	// Validate
	uint32_t timestamp;
	State::getInstance().get(STATE_TIME, timestamp);
#ifdef BACKGROUND_ADV_VERBOSE
	uint16_t timestampRounded = (timestamp >> 7) & 0x0000FFFF;
	LOGd("validation=%u time=%u %u", decryptedPayload[0], timestamp, timestampRounded);
#endif

	// TODO: validate with time
	if (decryptedPayload[0] != 0xCAFE){
		return;
	}

	backgroundAdvertisement.data = (uint8_t*)decryptedPayload;
	backgroundAdvertisement.dataSize = sizeof(decryptedPayload);

	handleBackgroundAdvertisement(&backgroundAdvertisement);
}

void BackgroundAdvertisementHandler::handleBackgroundAdvertisement(evt_adv_background_t* backgroundAdvertisement) {
	if (backgroundAdvertisement->dataSize != sizeof(uint16_t) * 2) {
		return;
	}
	uint16_t* decryptedPayload = (uint16_t*)(backgroundAdvertisement->data);
#ifdef BACKGROUND_ADV_VERBOSE
	logSerial(SERIAL_DEBUG, "bg adv: ");
	_logSerial(SERIAL_DEBUG, "protocol=%u sphereId=%u rssi=%i ", backgroundAdvertisement->protocol, backgroundAdvertisement->sphereId, backgroundAdvertisement->rssi);
	_logSerial(SERIAL_DEBUG, "payload=[%u %u] address=", decryptedPayload[0], decryptedPayload[1]);
	BLEutil::printAddress(backgroundAdvertisement->macAddress, BLE_GAP_ADDR_LEN);
#endif



	evt_adv_background_payload_t payload;
	payload.locationId = (decryptedPayload[1] >> (16-6)) & 0x3F;
	payload.profileId =  (decryptedPayload[1] >> (16-6-3)) & 0x07;
	payload.rssiOffset = (decryptedPayload[1] >> (16-6-3-4)) & 0x0F;
	payload.flags =      (decryptedPayload[1] >> (16-6-3-4-3)) & 0x07;
	payload.flags = payload.flags << 5; // So that the first bit is bit 0.
	backgroundAdvertisement->data = (uint8_t*)(&payload);
	backgroundAdvertisement->dataSize = sizeof(payload);
	adjustRssi(backgroundAdvertisement, payload);
	LOGd("validation=%u locationId=%u profileId=%u rssiOffset=%u flags=%u rssi=%i", decryptedPayload[0], payload.locationId, payload.profileId, payload.rssiOffset, payload.flags, backgroundAdvertisement->rssi);
	EventDispatcher::getInstance().dispatch(EVT_ADV_BACKGROUND_PARSED, backgroundAdvertisement, sizeof(*backgroundAdvertisement));
}

void BackgroundAdvertisementHandler::adjustRssi(evt_adv_background_t* backgroundAdvertisement, const evt_adv_background_payload_t& payload) {
	int16_t rssi = backgroundAdvertisement->rssi;
	rssi += ((int16_t)payload.rssiOffset - 8) * 4;
	if (rssi > -1) {
		rssi = -1;
	}
	if (rssi < -127) {
		rssi = -127;
	}
	backgroundAdvertisement->rssi = rssi;
}

void BackgroundAdvertisementHandler::handleEvent(uint16_t evt, void* data, uint16_t length) {
	switch(evt) {
	case EVT_DEVICE_SCANNED: {
		ble_gap_evt_adv_report_t* advReport = (ble_gap_evt_adv_report_t*)data;
		parseAdvertisement(advReport);
		break;
	}
	case EVT_ADV_BACKGROUND: {
		if (length != sizeof(evt_adv_background_t)) {
			return;
		}
		handleBackgroundAdvertisement((evt_adv_background_t*) data);
		break;
	}
	}
}
