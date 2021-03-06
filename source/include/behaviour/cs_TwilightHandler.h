/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 5, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <events/cs_EventListener.h>
#include <presence/cs_PresenceDescription.h>
#include <time/cs_Time.h>

#include <optional>


class TwilightHandler : public EventListener {
    public:
    /**
     * Computes the twilight state of this crownstone based on
     * the stored behaviours, and then dispatches an event.
     * 
     * Events:
     * - STATE_TIME
     * - EVT_TIME_SET
     * - EVT_PRESENCE_MUTATION
     * - EVT_BEHAVIOURSTORE_MUTATION
     */
    virtual void handleEvent(event_t& evt) override;

    /**
     * Acquires the current time and presence information. 
     * Checks the intended state by looping over the active behaviours
     * and if the intendedState differs from previousIntendedState
     * dispatch an event to communicate a state update.
     * 
     * if time is not valid, aborts method execution and returns false.
     * returns true when value was updated, false else.
     */
    bool update();
    
    uint8_t getValue();

    private:
    /**
     * Given current time, query the behaviourstore and check
     * if there any valid ones. 
     * 
     * Returns a non-empty optional if a valid behaviour is found or
     * multiple agreeing behaviours have been found.
     * In this case its value contains the desired state value.
     */
    uint8_t computeIntendedState(Time currenttime);

    uint8_t previousIntendedState = 100;
};
