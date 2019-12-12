/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 5, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <behaviour/cs_TwilightHandler.h>
#include <time/cs_SystemTime.h>

void TwilightHandler::handleEvent(event_t& evt){
    switch(evt.type){
        case CS_TYPE::EVT_PRESENCE_MUTATION: {
            update();
            break;
        }
        case CS_TYPE::STATE_TIME:{
            update();
            break;
        }
        case CS_TYPE::EVT_TIME_SET: {
            update();
            break;
        }
        case CS_TYPE::EVT_BEHAVIOURSTORE_MUTATION:{
            update();
            break;
        }
        default:{
            // ignore other events
            break;
        }
    }
}

void TwilightHandler::update(){
    TimeOfDay time = SystemTime::now();

    auto intendedState = computeIntendedState(time);
    if(intendedState){
        if(previousIntendedState == intendedState){
            return;
        }

        previousIntendedState = intendedState;
        
        uint8_t intendedValue = intendedState.value();
        event_t behaviourStateChange(
            CS_TYPE::EVT_BEHAVIOUR_SWITCH_STATE,
            &intendedValue,
            sizeof(uint8_t)
        );

        // behaviourStateChange.dispatch();
    }
}

std::optional<uint8_t> TwilightHandler::computeIntendedState(TimeOfDay currenttime){
    // return minimal value among the valid twilights.
    return {};
}
