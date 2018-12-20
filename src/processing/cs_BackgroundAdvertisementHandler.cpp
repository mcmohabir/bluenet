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



	uint8_t* servicesMask;
	servicesMask = manufacturerData.data + BACKGROUND_SERVICES_MASK_HEADER_LEN;

	logSerial(SERIAL_DEBUG, "servicesMask: ");
	BLEutil::printArray(servicesMask, BACKGROUND_SERVICES_MASK_LEN); // Received as uint128, so bytes are reversed.

//	// Divide the data into parts, and do a bitwise majority vote.
//	uint64_t part1 = *((uint64_t*)servicesMask) & 0x3FFFFFFFFFF;
//	uint64_t part2 = (*((uint64_t*)servicesMask) << 42) & 0x3FFFFFFFFFF;
//	uint64_t part3 = (*((uint64_t*)servicesMask) << 84) & 0x3FFFFFFFFFF;
//	uint64_t result = ((part1 & part2) | (part2 & part3) | (part1 & part3));

//  // method: loop over bytes
//	uint8_t data[4] = { 183, 45, 203, 112 };
//	uint8_t result[2];
//
//	uint8_t bits;
//	uint8_t ind;
//	uint8_t shift;
//	uint8_t maskLeft;
//	uint8_t maskRight;
//
//	bits = 0;
//	ind = bits / 8;
//	shift = bits % 8;
//	maskLeft = 0xFF << shift;
//	maskRight = ~(0xFF >> shift);
//	uint8_t p1 = (data[ind] << shift) & maskLeft;
//	if (shift) {
//		p1 |= (data[ind+1] & maskRight) >> 8-shift;
//	}
//
//	bits = 10;
//	ind = bits / 8;
//	shift = bits % 8;
//	maskLeft = 0xFF << shift;
//	maskRight = ~(0xFF >> shift);
//	uint8_t p2 = (data[ind] << shift) & maskLeft;
//	if (shift) {
//		p2 |= (data[ind+1] & maskRight) >> 8-shift;
//	}
//
//	bits = 20;
//	ind = bits / 8;
//	shift = bits % 8;
//	maskLeft = 0xFF << shift;
//	maskRight = ~(0xFF >> shift);
//	uint8_t p3 = (data[ind] << shift) & maskLeft;
//	if (shift) {
//		p3 |= (data[ind+1] & maskRight) >> 8-shift;
//	}
//
//	result[0] = ((p1&p2) | (p2&p3) | (p1&p3));
//
//	printf("p1=%u p2=%u p3=%u\r\n", p1, p2, p3);
//
//
//	// other method: shift all at once
//	uint8_t data2[4] = { 45, 183, 112, 203 };
//	uint16_t left = ((uint16_t*)data2)[0];
//	uint16_t right = ((uint16_t*)data2)[1];
//
//	uint16_t part1 = (left >> (16-10)) & 0x03FF; // first 10 bits from left
//	uint16_t part2 = ((left & 0x3F) << 4) | ((right >> (16-4) & 0x0F)); // last 6 bits from left and first 4 bits from right
//	uint16_t part3 = ((right >> 2) & 0x03FF); // bits 5-14 from right
//
//	printf("part1=%u part2=%u part3=%u\r\n", part1, part2, part3);


//	uint128_t* data = (uint128_t*)servicesMask; // Shame, there's no uint128_t
//	uint64_t part1 = (data << 0)  & 0x03FFFFFFFFFF;
//	uint64_t part2 = (data << 42) & 0x03FFFFFFFFFF;
//	uint64_t part3 = (data << 84) & 0x03FFFFFFFFFF;
//	LOGd("part1=%u part2=%u part3=%u", part1, part2, part3);


	LOGd("servicesMask=%p", servicesMask);
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


	uint64_t part1 = (left >> (64-42))  & 0x03FFFFFFFFFF; // First 10 bits from left.
	uint64_t part2 = ((left & 0x3FFFFF) << 20) | ((right >> (64-20)) & 0x0FFFFF); // Last 64-42=22 bits from left, and first 42−(64−42)=20 bits from right.
	uint64_t part3 = (right >> 2) & 0x03FFFFFFFFFF; // Bits 21-62 from right.
	LOGd("part1=%llx part2=%llx part3=%llx", part1, part2, part3);

//	// Divide the data into parts, and do a bitwise majority vote, to correct for errors.
//	uint8_t resultData[6];
//	for (int i=0; i<6; ++i) {
//		uint8_t part1 = servicesMask << (0+i*8); // Doesn't work, maybe when servicesMake is uint128_t
//		uint8_t part2 = servicesMask << (42+i*8);
//		uint8_t part3 = servicesMask << (84+i*8);
//		resultData[i] = ((part1 & part2) | (part2 & part3) | (part1 & part3));
//	}

//	// Parse the resulting data.
//	BackgroundAdvertisement bgAdvData;
//	bgAdvData.protocol = resultData
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
