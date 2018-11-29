#include <ctime>

#include <esp_log.h>

#include <lwip/apps/sntp.h>

#include "TimeUpdate.h"

void TimeUpdate::logTime() {
    std::time_t now = std::time(nullptr);
    char show[64];
    std::strftime(show, sizeof show, "%c", std::localtime(&now));
    ESP_LOGI(name, "%s", show);
}

TimeUpdate::TimeUpdate(
    char const * name_,
    std::vector<std::string> & timeUpdateServers)	// SNTP will reference
:
    name(name_)
{
    ESP_LOGI(name, "start");
    logTime();
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    size_t index = 0;
    for (std::string & server: timeUpdateServers) {
	if (index < CONFIG_LWIP_DHCP_MAX_NTP_SERVERS) {
	    ESP_LOGI(name, "server %zd %s", index, server.c_str());
	    sntp_setservername(index, const_cast<char *>(server.c_str()));
	} else {
	    ESP_LOGW(name,
		"server %zd (>= CONFIG_LWIP_DHCP_MAX_NTP_SERVERS) %s",
		index, server.c_str());
	}
	++index;
    }
    sntp_init();
}

TimeUpdate::~TimeUpdate() {
    logTime();
    ESP_LOGI(name, "stop");
    sntp_stop();
}
