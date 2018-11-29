#include <algorithm>
#include <map>
#include <memory>

#include <esp_log.h>
#include <esp_event.h>

#include "sdkconfig.h"

#include "Event.h"

/* static */ Event::Reactor * Event::Reactor::reactor(nullptr);

/* static */ Event::Reactor * Event::Reactor::getReactor() {
    return reactor
	? reactor
	: (reactor = new Reactor());
}

/// map of event ids to presentable names
/// scraped from esp_event_legacy.h
std::map<system_event_id_t, char const *> map {
    {SYSTEM_EVENT_WIFI_READY,		" WIFI READY"},
    {SYSTEM_EVENT_SCAN_DONE,		" SCAN DONE"},
    {SYSTEM_EVENT_STA_START,		" STA START"},
    {SYSTEM_EVENT_STA_STOP,		" STA STOP"},
    {SYSTEM_EVENT_STA_CONNECTED,	" STA CONNECTED"},
    {SYSTEM_EVENT_STA_DISCONNECTED,	" STA DISCONNECTED"},
    {SYSTEM_EVENT_STA_AUTHMODE_CHANGE,	" STA AUTHMODE CHANGE"},
    {SYSTEM_EVENT_STA_GOT_IP,		" STA GOT IP"},
    {SYSTEM_EVENT_STA_LOST_IP,		" STA LOST IP"},
    {SYSTEM_EVENT_STA_WPS_ER_SUCCESS,	" STA WPS ER SUCCESS"},
    {SYSTEM_EVENT_STA_WPS_ER_FAILED,	" STA WPS ER FAILED"},
    {SYSTEM_EVENT_STA_WPS_ER_TIMEOUT,	" STA WPS ER TIMEOUT"},
    {SYSTEM_EVENT_STA_WPS_ER_PIN,	" STA WPS ER PIN"},
    {SYSTEM_EVENT_AP_START,		" AP START"},
    {SYSTEM_EVENT_AP_STOP,		" AP STOP"},
    {SYSTEM_EVENT_AP_STACONNECTED,	" AP STACONNECTED"},
    {SYSTEM_EVENT_AP_STADISCONNECTED,	" AP STADISCONNECTED"},
    {SYSTEM_EVENT_AP_STAIPASSIGNED,	" AP STAIPASSIGNED"},
    {SYSTEM_EVENT_AP_PROBEREQRECVED,	" AP PROBEREQRECVED"},
    {SYSTEM_EVENT_GOT_IP6,		" GOT IP6"},
    {SYSTEM_EVENT_ETH_START,		" ETH START"},
    {SYSTEM_EVENT_ETH_STOP,		" ETH STOP"},
    {SYSTEM_EVENT_ETH_CONNECTED,	" ETH CONNECTED"},
    {SYSTEM_EVENT_ETH_DISCONNECTED,	" ETH DISCONNECTED"},
    {SYSTEM_EVENT_ETH_GOT_IP,		" ETH GOT IP"}
};

esp_err_t Event::Reactor::reactTo(system_event_t const * event) {
    system_event_id_t id = event->event_id;
    char const * name = "";
    auto it = map.find(id);
    if (it != map.end()) {
	name = it->second;
    }
    ESP_LOGI("Event", "Reactor reactTo %d%s", id, name);
    auto idIt = idMap.find(id);
    if (idIt == idMap.end()) {
	return ESP_OK;
    } else {
	ESP_LOGI("Event", "Reactor reactTo %d%s actions", id, name);
	ObserverMap * observerMap = idIt->second;
	esp_err_t result = ESP_OK;
	for (std::pair<Observer *, Action> element: *observerMap) {
	    ESP_LOGI("Event", "Reactor reactTo %d%s action", id, name);
	    esp_err_t err = element.second(event);
	    if (result < err) result = err;
	}
	return result;
    }
}

/* static */ esp_err_t Event::Reactor::reactToThat(
	void * that, system_event_t * event) {
    return static_cast<Event::Reactor *>(that)->reactTo(event);
}

Event::Reactor::Reactor() {
    ESP_LOGI("Event", "Reactor esp_event_loop_init");
    if (ESP_OK != esp_event_loop_init(reactToThat, this)) {
	ESP_LOGE("Event", "Reactor esp_event_loop_set_cb");
	esp_event_loop_set_cb(reactToThat, this);
    }
}

Event::Reactor::~Reactor() {
    ESP_LOGE("Event", "~Reactor esp_event_loop_set_cb nullptr");
    esp_event_loop_set_cb(nullptr, nullptr);
}

Event::Observer::Observer(system_event_id_t id_, Action && action)
:
    id(id_)
{
    ESP_LOGI("Event", "Observer %d", id);
    Reactor * reactor = Reactor::getReactor();
    auto idIt = reactor->idMap.find(id);
    ObserverMap * observerMap;
    if (idIt != reactor->idMap.end()) {
	observerMap = idIt->second;
    } else {
	reactor->idMap[id] = observerMap = new ObserverMap();
    }
    (*observerMap)[this] = std::move(action);
}

Event::Observer::~Observer() {
    ESP_LOGE("Event", "~Observer %d", id);
    Reactor * reactor = Reactor::getReactor();
    auto idIt = reactor->idMap.find(id);
    if (idIt != reactor->idMap.end()) {
	ObserverMap * observerMap = idIt->second;
	auto observerIt = observerMap->find(this);
	if (observerIt != observerMap->end()) {
	    observerMap->erase(observerIt);
	    if (!observerMap->size()) {
		reactor->idMap.erase(id);
		delete observerMap;
	    }
	}
    }
}
