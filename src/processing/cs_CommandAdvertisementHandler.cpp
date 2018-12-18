/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 14, 2018
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "processing/cs_CommandAdvertisementHandler.h"
#include "drivers/cs_Serial.h"
#include "util/cs_Utils.h"
#include "events/cs_EventDispatcher.h"
#include "ble/cs_Nordic.h"
#include "events/cs_EventTypes.h"
#include "processing/cs_EncryptionHandler.h"
#include "processing/cs_CommandHandler.h"

CommandAdvertisementHandler::CommandAdvertisementHandler() {
	EventDispatcher::getInstance().addListener(this);
}

void CommandAdvertisementHandler::parseAdvertisement(ble_gap_evt_adv_report_t* advReport) {
//	logSerial(SERIAL_DEBUG, "adv: ");
//	_logSerial(SERIAL_DEBUG, "rssi=%i addr_type=%u MAC=", advReport->rssi, advReport->peer_addr.addr_type);
//	BLEutil::printAddress((uint8_t*)(advReport->peer_addr.addr), BLE_GAP_ADDR_LEN);
// peer_addr.addr_type:
//#define BLE_GAP_ADDR_TYPE_PUBLIC                        0x00 /**< Public address. */
//#define BLE_GAP_ADDR_TYPE_RANDOM_STATIC                 0x01 /**< Random Static address. */
//#define BLE_GAP_ADDR_TYPE_RANDOM_PRIVATE_RESOLVABLE     0x02 /**< Private Resolvable address. */
//#define BLE_GAP_ADDR_TYPE_RANDOM_PRIVATE_NON_RESOLVABLE 0x03 /**< Private Non-Resolvable address. */

// scan_rsp: 1 if data is scan response, ignore type if so.

// type:
//#define BLE_GAP_ADV_TYPE_ADV_IND          0x00   /**< Connectable undirected. */
//#define BLE_GAP_ADV_TYPE_ADV_DIRECT_IND   0x01   /**< Connectable directed. */
//#define BLE_GAP_ADV_TYPE_ADV_SCAN_IND     0x02   /**< Scannable undirected. */
//#define BLE_GAP_ADV_TYPE_ADV_NONCONN_IND  0x03   /**< Non connectable undirected. */

// dlen: len of data
// data: uint8_t[]

	uint32_t errCode;
	data_t services16bit;
	errCode = BLEutil::findAdvType(BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE, advReport->data, advReport->dlen, &services16bit);
	if (errCode != ERR_SUCCESS) {
		return;
	}
	data_t services128bit;
	errCode = BLEutil::findAdvType(BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_COMPLETE, advReport->data, advReport->dlen, &services128bit);
	if (errCode != ERR_SUCCESS) {
		return;
	}

	logSerial(SERIAL_DEBUG, "16bit services: ");
	BLEutil::printArray(services16bit.data, services16bit.len);

	logSerial(SERIAL_DEBUG, "128bit services: ");
	BLEutil::printArray(services128bit.data, services128bit.len);

	if (services16bit.len < (CMD_ADV_NUM_SERVICES_16BIT * sizeof(uint16_t))) {
		return;
	}

	CommandAdvertisementHeader header = CommandAdvertisementHeader();
	bool foundSequences[CMD_ADV_NUM_SERVICES_16BIT] = { false };
	// Fill the struct with data from the 4 service UUIDs.
	// Keep up which sequence numbers have been handled.
	for (int i=0; i < CMD_ADV_NUM_SERVICES_16BIT; ++i) {
		uint16_t serviceUuid = ((uint16_t*)services16bit.data)[i];
		LOGd("uuid=%u", serviceUuid);
		uint8_t sequence = (serviceUuid >> 14) & 0x0003;
		foundSequences[sequence] = true;
		switch (sequence) {
		case 0:
			header.sequence0 = sequence;
			header.protocol =    (serviceUuid >> 6)  & 0x00FF;
			header.accessLevel = (serviceUuid >> 2)  & 0x000F;
			header.profile =     (serviceUuid >> 0)  & 0x0003;
			break;
		case 1:
			header.sequence1 = sequence;
			header.type =        (serviceUuid >> 12) & 0x0003;
			header.payload =     (serviceUuid >> 0)  & 0x0FFF;
			break;
		case 2:
			header.sequence2 = sequence;
			header.sphereId =    (serviceUuid >> 6)  & 0x00FF;
			header.locationId =  (serviceUuid >> 0)  & 0x003F;
			break;
		case 3:
			header.sequence3 = sequence;
			header.time =        (serviceUuid >> 0)  & 0x3FFF;
			break;
		}
	}
	for (int i=0; i < CMD_ADV_NUM_SERVICES_16BIT; ++i) {
		if (!foundSequences[i]) {
			return;
		}
	}


//	uint8_t* pHeader = (uint8_t*)&header;
//	LOGd("header pointer=%p size=%u", pHeader, sizeof(header));
//	_logSerial(SERIAL_DEBUG, "data=");
//	for (int i=0; i<8; ++i) {
//		uint8_t* ptr = pHeader + i;
//		_logSerial(SERIAL_DEBUG, "%u%u%u%u %u%u%u%u ",
//				((*ptr) & 0x80) >> 7,
//				((*ptr) & 0x40) >> 6,
//				((*ptr) & 0x20) >> 5,
//				((*ptr) & 0x10) >> 4,
//				((*ptr) & 0x08) >> 3,
//				((*ptr) & 0x04) >> 2,
//				((*ptr) & 0x02) >> 1,
//				((*ptr) & 0x01) >> 0
//				);
//	}
//	_logSerial(SERIAL_DEBUG, SERIAL_CRLF);
	LOGd("  protocol=%u accessLevel=%u profile=%u", header.protocol, header.accessLevel, header.profile);
	LOGd("  type=%u payload=%u", header.type, header.payload);
	LOGd("  sphereId=%u locationId=%u time=%u", header.sphereId, header.locationId, header.time);


	uint8_t decryptedData[16];
	EncryptionAccessLevel accessLevel;
	switch(header.accessLevel) {
	case 0:
		accessLevel = ADMIN;
		break;
	case 1:
		accessLevel = MEMBER;
		break;
	case 2:
		accessLevel = GUEST;
		break;
//	case 100:
//		accessLevel = SETUP;
//		break;
	default:
		accessLevel = NOT_SET;
		break;
	}

	uint8_t type =  services128bit.data[4];
	uint16_t length = services128bit.len - 5;
	uint8_t* commandData = services128bit.data + 5;
	CommandHandlerTypes commandType = CMD_UNKNOWN;
	switch (type) {
	case 1:
		commandType = CMD_ADV_MULTISWITCH;
		break;
	}

	if (commandType == CMD_UNKNOWN) {
		return;
	}
	CommandHandler::getInstance().handleCommand(commandType, commandData, length, accessLevel);

	if (!EncryptionHandler::getInstance().decrypt(services128bit.data, services128bit.len, decryptedData, 16, accessLevel)) {
		return;
	}
	logSerial(SERIAL_DEBUG, "decrypted data: ");
	BLEutil::printArray(decryptedData, 16);
}

void CommandAdvertisementHandler::handleEvent(uint16_t evt, void* data, uint16_t length) {
	switch(evt) {
	case EVT_DEVICE_SCANNED: {
		ble_gap_evt_adv_report_t* advReport = (ble_gap_evt_adv_report_t*)data;
		parseAdvertisement(advReport);
		break;
	}
	}
}
