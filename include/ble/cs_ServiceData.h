/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 4, 2016
 * License: LGPLv3+
 */
#pragma once

#include <events/cs_EventListener.h>
#include <events/cs_EventDispatcher.h>
#include <storage/cs_Settings.h>
#include <storage/cs_State.h>
#include "drivers/cs_Timer.h"
#include "cfg/cs_Config.h"

#include <cstring>

#if BUILD_MESHING == 1
#include <protocol/cs_MeshMessageTypes.h>
#endif

#define SERVICE_DATA_PROTOCOL_VERSION 1

//! bitmask map (if bit is true, then):
//! [0] New Data Available
//! [1] ID shown is from external Crownstone
//! [2] ....
//! [3] ....
//! [4] ....
//! [5] ....
//! [6] ....
//! [7] operation mode setup
enum ServiceBitmask {
	NEW_DATA_AVAILABLE    = 0,
	SHOWING_EXTERNAL_DATA = 1,
	RESERVED1             = 2,
	RESERVED2             = 3,
	RESERVED3             = 4,
	RESERVED4             = 5,
	RESERVED5             = 6,
	SETUP_MODE_ENABLED    = 7
};

class ServiceData : EventListener {

public:
	ServiceData();

	void updatePowerUsage(int32_t powerUsage) {
		_params.powerUsage = powerUsage;
	}

	void updateAccumulatedEnergy(int32_t accumulatedEnergy) {
		_params.accumulatedEnergy = accumulatedEnergy;
	}

	void updateCrownstoneId(uint16_t crownstoneId) {
		_params.crownstoneId = crownstoneId;
	}

	void updateSwitchState(uint8_t switchState) {
		_params.switchState = switchState;
	}

	void updateEventBitmask(uint8_t bitmask) {
		_params.eventBitmask = bitmask;
	}

	void updateTemperature(int8_t temperature) {
		_params.temperature = temperature;
	}

	void updateAdvertisement();

	void updateEventBitmask(uint8_t bit, bool set) {
		if (set) {
			_params.eventBitmask |= 1 << bit;
		} else {
			_params.eventBitmask &= ~(1 << bit);
		}
	}

	uint8_t* getArray() {
		if (Settings::getInstance().isSet(CONFIG_ENCRYPTION_ENABLED) && _operationMode != OPERATION_MODE_SETUP) {
			return _encryptedArray;
		}
		else {
			return _array;
		}

	}

	uint16_t getArraySize() {
		return sizeof(_params);
	}

	void sendMeshState(bool event);

private:

	app_timer_t    _updateTimerData;
	app_timer_id_t _updateTimerId;

	uint8_t _operationMode;
	bool _connected;

	/* Static function for the timeout */
	static void staticTimeout(ServiceData *ptr) {
		ptr->updateAdvertisement();
	}

	union {
		struct __attribute__((packed)) {
			uint8_t  protocolVersion;
			uint16_t crownstoneId;
			uint8_t  switchState;
			uint8_t  eventBitmask;
			int8_t   temperature;
			int32_t  powerUsage;
			int32_t  accumulatedEnergy;
			uint8_t  rand;
			uint16_t counter;
		} _params;
		uint8_t _array[sizeof(_params)] = {};
	};

	union {
		struct __attribute__((packed)) {
			uint8_t  protocolVersion;
			uint8_t  payload[16];
		} _encryptedParams;
		uint8_t _encryptedArray[sizeof(_params)] = {};
	};

	void handleEvent(uint16_t evt, void* p_data, uint16_t length);

#if BUILD_MESHING == 1
	app_timer_t    _meshStateTimerData;
	app_timer_id_t _meshStateTimerId;

	/* Static function for the timeout */
	static void meshStateTick(ServiceData *ptr) {
		ptr->sendMeshState(false);
	}

	state_item_t _lastStateChangeMessage;
#endif

};

