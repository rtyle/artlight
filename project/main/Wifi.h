#pragma once

#include <cstring>

#include "esp_wifi.h"

#include "Event.h"
#include "Timer.h"

// https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/wifi.html
class Wifi {
private:
    char const *	name;
    Timer		reconnectTimer;
    Event::Handler	staStartHandler;
    Event::Handler	staGotIpHandler;
    Event::Handler	staDisconnectedHandler;

    void reconnect();

public:

    /// A Wifi::InitConfig is a wifi_init_config_t
    /// with convenience setter methods that can be chained together.
    /// The result can be used to call esp_wifi_init().
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
	setter(cache_tx_buf_num)
	setter(csi_enable)
	setter(ampdu_rx_enable)
	setter(ampdu_tx_enable)
	setter(amsdu_tx_enable)
	setter(nvs_enable)
	setter(nano_enable)
	setter(rx_ba_win)
	setter(wifi_task_core_id)
	setter(beacon_max_len)
	setter(mgmt_sbuf_num)
	setter(feature_caps)
	setter(sta_disconnected_pm)
	setter(magic)
	#undef setter
    };

    /// A Wifi::InitConfig is a wifi_ap_config_t
    /// with convenience setter methods that can be chained together.
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

    /// A Wifi::InitConfig is a wifi_sta_config_t
    /// with convenience setter methods that can be chained together.
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
	TickType_t reconnectTimeout = pdMS_TO_TICKS(60 * 1000));

    void start();
};
