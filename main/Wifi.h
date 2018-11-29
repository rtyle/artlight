#ifndef Wifi_h_
#define Wifi_h_

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

#include <esp_wifi.h>

#include "Event.h"
#include "Timer.h"

// https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/wifi.html
class Wifi {
private:
    char const *	name;
    Timer		reconnectTimer;
    Event::Observer	staStartObserver;
    Event::Observer	staGotIpObserver;
    Event::Observer	staDisconnectedObserver;

    void reconnect();

public:
    struct InitConfig : public wifi_init_config_t {
    public:
	static wifi_init_config_t const wifi_init_config_default;
	InitConfig() : wifi_init_config_t(wifi_init_config_default) {}
	#define setter(name) \
	    InitConfig & name##_(decltype(name) s) {name = s; return *this;}
	setter(event_handler)
	setter(osi_funcs)
	setter(wpa_crypto_funcs)
	setter(static_rx_buf_num)
	setter(dynamic_rx_buf_num)
	setter(tx_buf_type)
	setter(static_tx_buf_num)
	setter(dynamic_tx_buf_num)
	setter(csi_enable)
	setter(ampdu_rx_enable)
	setter(ampdu_tx_enable)
	setter(nvs_enable)
	setter(nano_enable)
	setter(tx_ba_win)
	setter(rx_ba_win)
	setter(wifi_task_core_id)
	setter(beacon_max_len)
	setter(magic)
	#undef setter
    };

    struct ApConfig : public wifi_ap_config_t {
    public:
	ApConfig() : wifi_ap_config_t {} {}
	#define setter(name) ApConfig & name##_(decltype(name) s, size_t z) {\
	    std::memcpy(name, s, sizeof name < z ? sizeof name : z); return *this;}
	setter(ssid)
	setter(password)
	#undef setter
	#define setter(name) \
	    ApConfig & name##_(decltype(name) s) {name = s; return *this;}
	setter(ssid_len)
	setter(channel)
	setter(authmode)
	setter(ssid_hidden)
	setter(max_connection)
	setter(beacon_interval)
	#undef setter
    };

    struct StaConfig : public wifi_sta_config_t {
    public:
	StaConfig() : wifi_sta_config_t {} {}
	#define setter(name) StaConfig & name##_(decltype(name) s, size_t z) {\
	    std::memcpy(name, s, sizeof name < z ? sizeof name : z); return *this;}
	setter(ssid)
	setter(password)
	setter(bssid)
	#undef setter
	#define setter(name) \
	    StaConfig & name##_(decltype(name) s) {name = s; return *this;}
	setter(scan_method)
	setter(bssid_set)
	setter(channel)
	setter(listen_interval)
	setter(sort_method)
	setter(threshold)
	#undef setter
    };

    Wifi(
	char const * name,
	TickType_t reconnectTimeout = pdMS_TO_TICKS(8 * 60 * 1000));

    void start();
};

#endif
