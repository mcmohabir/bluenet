#include <localization/cs_LocalizationHandler.h>



void LocalizationHandler::handleEvent(event_t& evt){

    bool fromMesh = false;
    switch(evt.type) {
    case CS_TYPE::EVT_ADV_BACKGROUND_PARSED: {

        adv_background_parsed_t* parsed_adv_ptr = reinterpret_cast<TYPIFY(EVT_ADV_BACKGROUND_PARSED)*>(evt.data);

        adv_rssi_t rssi_message;
        rssi_message.rssi = parsed_adv_ptr->adjustedRssi;
        rssi_message.macAddress = &parsed_adv_ptr->macAddress;
        LOGi("received macAddress=%i  rssi=%u", rssi_message.macAddress, rssi_message.rssi);



        break;
    }
    default:
        return;k
}

// event_t event(CS_TYPE::EVT_MESH_TIME, payload, payloadSize);
// EventDispatcher::getInstance().dispatch(event);
