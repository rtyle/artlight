#include <cstring>

#include <esp_log.h>
#include <esp_wifi.h>

#include <tcpip_adapter.h>

#include "Wifi.h"

#include "dump.h"

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
    name(name_),
    reconnectTimer(name, reconnectTimeout, false, [this](){this->reconnect();}),
    staStartObserver(SYSTEM_EVENT_STA_START,
	    [this](system_event_t const * event)->esp_err_t{
        reconnect();
        return ESP_OK;
    }),
    staGotIpObserver(SYSTEM_EVENT_STA_GOT_IP,
	    [this](system_event_t const * event)->esp_err_t{
	// we will get here when esp_wifi_connect succeeds -
	// whether this attempt was done when our one-shot
	// reconnectTimer expired or not.
	// in any case, stop our reconnectTimer.
	// if it is running and we can't stop it in time (it expires)
	// there will be no harm in it invoking reconnect.
        reconnectTimer.stop();
        return ESP_OK;
    }),
    staDisconnectedObserver(SYSTEM_EVENT_STA_DISCONNECTED,
	    [this](system_event_t const * event)->esp_err_t{
	reconnectTimer.start();
	return ESP_OK;
    })
{}

void Wifi::start() {
    // initialization
    {
	// create the Wi-Fi driver task
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    }

    // configuration
    {
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

	// inspect/configure wifi station interface
	// to expect/allow for separate/non-volatile-storage provisioning:
	// the ssid must not be the wildcard value (zero length)
	// in order for a connection to be attempted
	// and failure to be reported (SYSTEM_EVENT_STA_DISCONNECTED)
	wifi_config_t wifi_config;
	ESP_ERROR_CHECK(esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_config));
	if (!*wifi_config.sta.ssid) {
	    std::strncpy(reinterpret_cast<char *>(wifi_config.sta.ssid),
		"-", sizeof wifi_config.sta.ssid);
	    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	}
    }

    ESP_ERROR_CHECK(esp_wifi_start());
}
