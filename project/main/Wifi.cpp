#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"

#include "Wifi.h"

/* static */ wifi_init_config_t const Wifi::InitConfig::wifi_init_config_default
    = WIFI_INIT_CONFIG_DEFAULT();

/// Ask the wifi driver task to attempt to (re)connect.
/// This will result in either a
/// SYSTEM_EVENT_STA_GOT_IP event (success)
/// or SYSTEM_EVENT_STA_DISCONNECTED (failure).
void Wifi::reconnect() {
    ESP_LOGI(name, "reconnect");
    ESP_ERROR_CHECK(esp_wifi_connect());
}

Wifi::Wifi(char const * name_, TickType_t reconnectTimeout) :
    name{name_},
    reconnectTimer{name, reconnectTimeout, true, [this](){this->reconnect();}},
    staStartHandler{nullptr, WIFI_EVENT, WIFI_EVENT_STA_START,
	    [this](esp_event_base_t, int32_t, void *){
        reconnect();
    }},
    staGotIpHandler{nullptr, IP_EVENT, IP_EVENT_STA_GOT_IP,
	    [this](esp_event_base_t, int32_t, void *){
	// we will get here when esp_wifi_connect succeeds -
	// whether this attempt was done when our one-shot
	// reconnectTimer expired or not.
	// in any case, stop our reconnectTimer.
	// if it is running and we can't stop it in time (it expires)
	// there will be no harm in it invoking reconnect.
        reconnectTimer.stop();
    }},
    staDisconnectedHandler{nullptr, WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,
	    [this](esp_event_base_t, int32_t, void *){
	reconnectTimer.start();
    }}
{}

void Wifi::start() {
    // initialization
    {
	esp_netif_create_default_wifi_sta();
	esp_netif_create_default_wifi_ap();
	InitConfig initConfig{};
	ESP_ERROR_CHECK(esp_wifi_init(&initConfig));
    }

    // configuration
    {
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

	// inspect/configure wifi station interface
	// to expect/allow for separate/non-volatile-storage provisioning:
	// the ssid must not be the wildcard value (zero length)
	// in order for a connection to be attempted
	// and failure to be reported (WIFI_EVENT_STA_DISCONNECTED)
	wifi_config_t wifi_config;
	ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_config));
	if (!*wifi_config.sta.ssid) {
	    std::strncpy(reinterpret_cast<char *>(wifi_config.sta.ssid),
		"-", sizeof wifi_config.sta.ssid);
	    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
	}
    }

    ESP_ERROR_CHECK(esp_wifi_start());
}
