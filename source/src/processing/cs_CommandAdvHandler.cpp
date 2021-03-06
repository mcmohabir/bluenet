/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 14, 2018
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "processing/cs_CommandAdvHandler.h"
#include "drivers/cs_Serial.h"
#include "util/cs_Utils.h"
#include "events/cs_EventDispatcher.h"
#include "common/cs_Types.h"
#include "processing/cs_EncryptionHandler.h"
#include "storage/cs_State.h"

// Defines to enable extra debug logs.
// #define COMMAND_ADV_DEBUG
// #define COMMAND_ADV_VERBOSE

#ifdef COMMAND_ADV_DEBUG
#define LOGCommandAdvDebug LOGd
#else
#define LOGCommandAdvDebug LOGnone
#endif

#ifdef COMMAND_ADV_VERBOSE
#define LOGCommandAdvVerbose LOGd
#else
#define LOGCommandAdvVerbose LOGnone
#endif

#if CMD_ADV_CLAIM_TIME_MS / TICK_INTERVAL_MS > 250
#error "Timeout counter will overflow."
#endif

constexpr int8_t RSSI_LOG_THRESHOLD = -40;

CommandAdvHandler::CommandAdvHandler() {
}

void CommandAdvHandler::init() {
	State::getInstance().get(CS_TYPE::CONFIG_SPHERE_ID, &_sphereId, sizeof(_sphereId));
	EventDispatcher::getInstance().addListener(this);
}

void CommandAdvHandler::parseAdvertisement(scanned_device_t* scannedDevice) {
//	_log(SERIAL_DEBUG, "adv: ");
//	_log(SERIAL_DEBUG, "rssi=%i addressType=%u MAC=", scannedDevice->rssi, scannedDevice->addressType);
//	BLEutil::printAddress((uint8_t*)(scannedDevice->address), MAC_ADDRESS_LEN);
// addressType:
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
	cs_data_t services16bit;
	errCode = BLEutil::findAdvType(BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE, scannedDevice->data, scannedDevice->dataSize, &services16bit);
	if (errCode != ERR_SUCCESS) {
		return;
	}
	cs_data_t services128bit;
	errCode = BLEutil::findAdvType(BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_COMPLETE, scannedDevice->data, scannedDevice->dataSize, &services128bit);
	if (errCode != ERR_SUCCESS) {
		return;
	}

	if(scannedDevice->rssi > RSSI_LOG_THRESHOLD) {
#ifdef COMMAND_ADV_VERBOSE
		LOGCommandAdvVerbose("rssi=%i", scannedDevice->rssi);
		_log(SERIAL_DEBUG, "16bit services: ");
		BLEutil::printArray(services16bit.data, services16bit.len);
#endif
#ifdef COMMAND_ADV_VERBOSE
		_log(SERIAL_DEBUG, "128bit services: ");
		BLEutil::printArray(services128bit.data, services128bit.len); // Received as uint128, so bytes are reversed.
#endif
	}

	if (services16bit.len < (CMD_ADV_NUM_SERVICES_16BIT * sizeof(uint16_t))) {
		return;
	}

	command_adv_header_t header = command_adv_header_t();
	bool foundSequences[CMD_ADV_NUM_SERVICES_16BIT] = { false };
	uint8_t nonce[CMD_ADV_NUM_SERVICES_16BIT * sizeof(uint16_t)];
	uint16_t encryptedPayloadRC5[2];
	// Fill the struct with data from the 4 service UUIDs.
	// Keep up which sequence numbers have been handled.
	// Fill the nonce with the service data in the correct order.
	for (int i=0; i < CMD_ADV_NUM_SERVICES_16BIT; ++i) {
		uint16_t serviceUuid = ((uint16_t*)services16bit.data)[i];
		LOGCommandAdvVerbose("uuid=%u", serviceUuid);
		uint8_t sequence = (serviceUuid >> (16-2)) & 0x0003;
		foundSequences[sequence] = true;
		switch (sequence) {
		case 0:
//			header.sequence0 = sequence;                               // 2 bits sequence
			header.protocol =    (serviceUuid >> (16-2-3)) & 0x07;     // 3 bits protocol
			header.sphereId =    (serviceUuid >> (16-2-3-8)) & 0xFF;   // 8 bits sphereId
			header.accessLevel = (serviceUuid >> (16-2-3-8-3)) & 0x07; // 3 bits access level
			memcpy(nonce+0, &serviceUuid, sizeof(uint16_t));
			break;
		case 1:
//			header.sequence1 = sequence;                                  // 2 bits sequence
//			header.reserved = ;                                           // 2 bits reserved
			header.deviceToken =      (serviceUuid >> (16-2-2-8)) & 0xFF; // 8 bits device token
			encryptedPayloadRC5[0] = ((serviceUuid >> (16-2-2-8-4)) & 0x0F) << (16-4);    // Last 4 bits of this service UUID are first 4 bits of encrypted payload[0].
			memcpy(nonce+2, &serviceUuid, sizeof(uint16_t));
			break;
		case 2:
//			header.sequence2 = sequence;
			encryptedPayloadRC5[0] += ((serviceUuid >> (16-2-12)) & 0x0FFF) << (16-4-12); // Bits 2 - 13 of this service UUID are last 12 bits of encrypted payload[0].
			encryptedPayloadRC5[1] = ((serviceUuid >> (16-2-12-2)) & 0x03) << (16-2);     // Last 2 bits of this service UUID are first 2 bits of encrypted payload[1].
			memcpy(nonce+4, &serviceUuid, sizeof(uint16_t));
			break;
		case 3:
//			header.sequence3 = sequence;
			encryptedPayloadRC5[1] += ((serviceUuid >> (16-2-14)) & 0x3FFF) << (16-2-14); // Last 14 bits of this service UUID are last 14 bits of encrypted payload[1].
			memcpy(nonce+6, &serviceUuid, sizeof(uint16_t));
			break;
		}
	}
	for (int i=0; i < CMD_ADV_NUM_SERVICES_16BIT; ++i) {
		if (!foundSequences[i]) {
			if(scannedDevice->rssi > RSSI_LOG_THRESHOLD) { LOGCommandAdvVerbose("Missing UUID with sequence %i", i);}
			return;
		}
	}

	if (header.sphereId != _sphereId) {
		if(scannedDevice->rssi > RSSI_LOG_THRESHOLD) {LOGCommandAdvVerbose("Wrong sphereId got=%u stored=%u", header.sphereId, _sphereId);}
		return;
	}

	cs_data_t nonceData;
	nonceData.data = nonce;
	nonceData.len = sizeof(nonce);

	uint16_t decryptedPayloadRC5[2];
	bool validated = handleEncryptedCommandPayload(scannedDevice, header, nonceData, services128bit, encryptedPayloadRC5, decryptedPayloadRC5);
	if (validated) {
		handleDecryptedRC5Payload(scannedDevice, header, decryptedPayloadRC5);
	}
}

int CommandAdvHandler::checkSimilarCommand(uint8_t deviceToken, uint32_t encryptedData) {
	int index = -1;
	for (int i=0; i<CMD_ADV_MAX_CLAIM_COUNT; ++i) {
		if (_claims[i].deviceToken == deviceToken) {
			if (_claims[i].timeoutCounter && _claims[i].encryptedData == encryptedData) {
				LOGCommandAdvDebug("Ignore similar payload");
				return -2;
			}
			index = i;
			break;
		}
	}
	return index;
}

bool CommandAdvHandler::claim(uint8_t deviceToken, uint32_t encryptedData, int index) {
	if (index == -1) {
		// Check if there's a spot to claim.
		for (int i=0; i<CMD_ADV_MAX_CLAIM_COUNT; ++i) {
			if (!_claims[i].timeoutCounter) {
				index = i;
				break;
			}
		}
	}
	if (index != -1) {
		// Claim the spot
		_claims[index].deviceToken = deviceToken;
		_claims[index].timeoutCounter = CMD_ADV_CLAIM_TIME_MS / TICK_INTERVAL_MS;
		_claims[index].encryptedData = encryptedData;
		return true;
	}
	LOGCommandAdvDebug("No more claim spots");
	return false;
}

void CommandAdvHandler::tickClaims() {
	for (int i=0; i<CMD_ADV_MAX_CLAIM_COUNT; ++i) {
		if (_claims[i].timeoutCounter) {
			--_claims[i].timeoutCounter;
		}
	}
}

uint32_t CommandAdvHandler::getPartOfEncryptedData(cs_data_t& encryptedPayload) {
	return *(uint32_t*)(encryptedPayload.data);
}

bool CommandAdvHandler::handleEncryptedCommandPayload(scanned_device_t* scannedDevice, const command_adv_header_t& header, const cs_data_t& nonce, cs_data_t& encryptedPayload, uint16_t encryptedPayloadRC5[2], uint16_t decryptedPayloadRC5[2]) {
	int claimIndex = checkSimilarCommand(header.deviceToken, getPartOfEncryptedData(encryptedPayload));
	if (claimIndex == -2) {
		LOGCommandAdvVerbose("Ignore already handled command");
		return false;
	}

	EncryptionAccessLevel accessLevel;
	switch(header.accessLevel) {
	case 0:
		accessLevel = ADMIN;
		break;
	case 1:
		accessLevel = MEMBER;
		break;
	case 2:
		accessLevel = BASIC;
		break;
	case 4:
		accessLevel = SETUP;
		break;
	default:
		accessLevel = NOT_SET;
		break;
	}
	if (accessLevel == NOT_SET) {
		LOGw("Invalid access level %u", header.accessLevel);
		return false;
	}

	// TODO: can decrypt to same buffer?
	uint8_t decryptedData[16];
	if (!EncryptionHandler::getInstance().decryptBlockCTR(encryptedPayload.data, encryptedPayload.len, decryptedData, 16, accessLevel, nonce.data, nonce.len)) {
		LOGCommandAdvVerbose("Decrypt failed");
		return false;
	}
#ifdef COMMAND_ADV_VERBOSE
	_log(SERIAL_DEBUG, "decrypted data: ");
	BLEutil::printArray(decryptedData, 16);
#endif

	uint32_t validationTimestamp = (decryptedData[0] << 0) | (decryptedData[1] << 8) | (decryptedData[2] << 16) | (decryptedData[3] << 24);
	TYPIFY(STATE_TIME) timestamp;
	State::getInstance().get(CS_TYPE::STATE_TIME, &timestamp, sizeof(timestamp));
	uint8_t typeInt = decryptedData[4];
	AdvCommandTypes type = (AdvCommandTypes)typeInt;
	uint16_t headerSize = sizeof(validationTimestamp) + sizeof(typeInt);
	uint16_t length = SOC_ECB_CIPHERTEXT_LENGTH - headerSize;
	uint8_t* commandData = decryptedData + headerSize;

	// TODO: validate with time.
	if (validationTimestamp != 0xCAFEBABE) {
		return false;
	}
	if (!decryptRC5Payload(encryptedPayloadRC5, decryptedPayloadRC5)) {
		return false;
	}
	// Validated, so from here on, return true.

	// Claim only after validation
	if (!claim(header.deviceToken, getPartOfEncryptedData(encryptedPayload), claimIndex)) {
		return true;
	}

	TYPIFY(CMD_CONTROL_CMD) controlCmd;
	controlCmd.type = CTRL_CMD_UNKNOWN;
	controlCmd.accessLevel = accessLevel;
	controlCmd.data = commandData;
	controlCmd.size = length;
	controlCmd.source.flagExternal = false;
	controlCmd.source.sourceId = CS_CMD_SOURCE_DEVICE_TOKEN + header.deviceToken;
	controlCmd.source.count = (decryptedPayloadRC5[0] >> 8) & 0xFF;
	LOGCommandAdvDebug("adv cmd type=%u", type);
	if (!EncryptionHandler::getInstance().allowAccess(getRequiredAccessLevel(type), accessLevel)) {
		LOGCommandAdvDebug("no access");
		return true;
	}


	switch (type) {
		case ADV_CMD_MULTI_SWITCH: {
			controlCmd.type = CTRL_CMD_MULTI_SWITCH;
			LOGd("send cmd type=%u sourceId=%u cmdCount=%u", controlCmd.type, controlCmd.source.sourceId, controlCmd.source.count);
			event_t event(CS_TYPE::CMD_CONTROL_CMD, &controlCmd, sizeof(controlCmd));
			event.dispatch();
			break;
		}
		case ADV_CMD_SET_TIME: {
			uint8_t flags;
			size16_t flagsSize = sizeof(flags);
			size16_t setTimeSize = sizeof(uint32_t);
			size16_t setSunTimeSize = 6; // 2 uint24
			if (length < flagsSize + setTimeSize + setSunTimeSize) {
				return true;
			}
			// First get flags
			flags = commandData[0];

			// Set time, if valid.
			if (BLEutil::isBitSet(flags, 0)) {
				controlCmd.type = CTRL_CMD_SET_TIME;
				controlCmd.data = commandData + flagsSize;
				controlCmd.size = setTimeSize;
				LOGd("send cmd type=%u sourceId=%u cmdCount=%u", controlCmd.type, controlCmd.source.sourceId, controlCmd.source.count);
				event_t eventSetTime(CS_TYPE::CMD_CONTROL_CMD, &controlCmd, sizeof(controlCmd));
				eventSetTime.dispatch();
			}

			// Set sun time, if valid.
			if (BLEutil::isBitSet(flags, 1)) {
				sun_time_t sunTime;
				sunTime.sunrise = (commandData[flagsSize + setTimeSize + 0] << 0) + (commandData[flagsSize + setTimeSize + 1] << 8) + (commandData[flagsSize + setTimeSize + 2] << 16);
				sunTime.sunset  = (commandData[flagsSize + setTimeSize + 3] << 0) + (commandData[flagsSize + setTimeSize + 4] << 8) + (commandData[flagsSize + setTimeSize + 5] << 16);
				LOGd("sunTime: %u %u", sunTime.sunrise, sunTime.sunset);
				controlCmd.type = CTRL_CMD_SET_SUN_TIME;
				controlCmd.data = (buffer_ptr_t)&sunTime;
				controlCmd.size = sizeof(sunTime);
				event_t eventSetSunTime(CS_TYPE::CMD_CONTROL_CMD, &controlCmd, sizeof(controlCmd));
				eventSetSunTime.dispatch();
			}
			break;
		}
		case ADV_CMD_SET_BEHAVIOUR_SETTINGS: {
			size16_t requiredSize = sizeof(TYPIFY(STATE_BEHAVIOUR_SETTINGS));
			if (length < requiredSize) {
				return true;
			}
			// Set state.
			cs_state_data_t stateData(CS_TYPE::STATE_BEHAVIOUR_SETTINGS, commandData, requiredSize);
			State::getInstance().set(stateData);
			// Send over mesh.
			event_t meshCmd(CS_TYPE::CMD_SEND_MESH_MSG_SET_BEHAVIOUR_SETTINGS, commandData, requiredSize);
			meshCmd.dispatch();
			break;
		}
		default:
			LOGd("Unkown adv cmd: %u", type);
			return true;
	}
	return true;
}

bool CommandAdvHandler::decryptRC5Payload(uint16_t encryptedPayload[2], uint16_t decryptedPayload[2]) {
	LOGCommandAdvVerbose("encrypted RC5=[%u %u]", encryptedPayload[0], encryptedPayload[1]);
	bool success = EncryptionHandler::getInstance().RC5Decrypt(encryptedPayload, sizeof(uint16_t) * 2, decryptedPayload, sizeof(uint16_t) * 2); // Can't use sizeof(encryptedPayload) as that returns size of pointer.
	LOGCommandAdvVerbose("decrypted RC5=[%u %u]", decryptedPayload[0], decryptedPayload[1]);
	return success;
}

void CommandAdvHandler::handleDecryptedRC5Payload(scanned_device_t* scannedDevice, const command_adv_header_t& header, uint16_t decryptedPayload[2]) {
	adv_background_t backgroundAdv;
	switch (header.protocol) {
	case 0:
		backgroundAdv.protocol = 0;
		break;
	default:
		return;
	}
	backgroundAdv.sphereId = header.sphereId;
	backgroundAdv.data = (uint8_t*)(decryptedPayload);
	backgroundAdv.dataSize = sizeof(uint16_t) * 2;
	backgroundAdv.macAddress = scannedDevice->address;
	backgroundAdv.rssi = scannedDevice->rssi;

	event_t event(CS_TYPE::EVT_ADV_BACKGROUND, &backgroundAdv, sizeof(backgroundAdv));
	EventDispatcher::getInstance().dispatch(event);
}

EncryptionAccessLevel CommandAdvHandler::getRequiredAccessLevel(const AdvCommandTypes type) {
	switch (type) {
		case ADV_CMD_MULTI_SWITCH:
			return BASIC;
		case ADV_CMD_SET_TIME:
		case ADV_CMD_SET_BEHAVIOUR_SETTINGS:
			return MEMBER;
		case ADV_CMD_UNKNOWN:
			return NOT_SET;
	}
	return NOT_SET;
}

void CommandAdvHandler::handleEvent(event_t & event) {
	switch(event.type) {
	case CS_TYPE::EVT_DEVICE_SCANNED: {
		TYPIFY(EVT_DEVICE_SCANNED)* scannedDevice = (TYPIFY(EVT_DEVICE_SCANNED)*)event.data;
		parseAdvertisement(scannedDevice);
		break;
	}
	case CS_TYPE::EVT_TICK: {
		tickClaims();
		break;
	}
	default:
		break;
	}
}
