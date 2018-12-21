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

	logSerial(SERIAL_DEBUG, "servicesMask: ");
	BLEutil::printArray(servicesMask, BACKGROUND_SERVICES_MASK_LEN);

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
	LOGd("left=%llx right=%llx", left, right);


	// Divide the data into 3 parts, and do a bitwise majority vote, to correct for errors.
	uint64_t part1 = (left >> (64-42))  & 0x03FFFFFFFFFF; // First 42 bits from left.
	uint64_t part2 = ((left & 0x3FFFFF) << 20) | ((right >> (64-20)) & 0x0FFFFF); // Last 64-42=22 bits from left, and first 42−(64−42)=20 bits from right.
	uint64_t part3 = (right >> 2) & 0x03FFFFFFFFFF; // Bits 21-62 from right.
	LOGd("part1=%llx part2=%llx part3=%llx", part1, part2, part3);
	uint64_t result = ((part1 & part2) | (part2 & part3) | (part1 & part3)); // The majority vote

	// Parse the resulting data.
	BackgroundAdvertisement bgAdvData;
	bgAdvData.protocol = (result >> (42-2)) & 0x03;
	bgAdvData.sphereId = (result >> (42-2-8)) & 0xFF;
	bgAdvData.encryptedData[0] = (result >> (42-2-8-16)) & 0xFFFF;
	bgAdvData.encryptedData[1] = (result >> (42-2-8-32)) & 0xFFFF;
	LOGd("protocol=%u sphereId=%u encryptedData=%u", bgAdvData.protocol, bgAdvData.sphereId, bgAdvData.encryptedData);

	uint16_t decryptedData[2];
//	EncryptionHandler::getInstance().RC5Decrypt(bgAdvData.encryptedData, 2*sizeof(uint16_t), decryptedData, 2*sizeof(uint16_t))
	EncryptionHandler::getInstance().RC5Decrypt(bgAdvData.encryptedData, sizeof(bgAdvData.encryptedData), decryptedData, sizeof(decryptedData));
	LOGd("decrypted=%u %u", decryptedData[0], decryptedData[1]);

	BackgroundAdvertisementPayload payload;
	payload.locationId = (decryptedData[1] >> (16-6)) & 0x3F;
	payload.profileId =  (decryptedData[1] >> (16-6-3)) & 0x07;
	payload.rssiOffset = (decryptedData[1] >> (16-6-3-4)) & 0x0F;
	payload.flags =      (decryptedData[1] >> (16-6-3-4-3)) & 0x07;
	LOGd("validation=%u locationId=%u profileId=%u rssiOffset=%u flags=%u", decryptedData[0], payload.locationId, payload.profileId, payload.rssiOffset, payload.flags);
}


void BackgroundAdvertisementHandler::handleEvent(uint16_t evt, void* data, uint16_t length) {
	switch(evt) {
	case EVT_DEVICE_SCANNED: {
		ble_gap_evt_adv_report_t* advReport = (ble_gap_evt_adv_report_t*)data;
		parseAdvertisement(advReport);
		break;
	}
	}
}
