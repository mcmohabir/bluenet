#include <events/cs_EventListener.h>

struct __attribute__((__packed__)) adv_rssi_t {
        int8_t rssi;
        uint8_t macAddress;
};


class LocalizationHandler{
public:
    void handleEvent(event_t& evt);




}
