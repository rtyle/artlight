#include <cstring>
#include <ctime>
#include <sstream>

#include "esp_log.h"

#include "esp_sntp.h"

#include "TimeUpdate.h"

static void time_sync_notification(struct timeval * tv) {
    std::tm tm;
    localtime_r(&tv->tv_sec, &tm);
    ESP_LOGI("timeUpdate", "%lu %04d-%02d-%02d %02d:%02d:%02d",
	tv->tv_sec,
	tm.tm_year + 1900,
	tm.tm_mon + 1,
	tm.tm_mday,
	tm.tm_hour,
	tm.tm_min,
	tm.tm_sec);
}

TimeUpdate::TimeUpdate(
    char const *	name_,
    KeyValueBroker &	keyValueBroker)
:
    name{name_},

    timeServers{},

    timeServersObserver(keyValueBroker, "timeServers", CONFIG_TIME_SERVERS,
	[this](char const * timeServers_){
	    // quiesce SNTP activity in the LwIP task while we change things
	    // there is no harm done if SNTP is not active
	    ESP_LOGI(name, "stop");
	    sntp_stop();

	    // forget/free old timeServers
	    for (auto e: timeServers) free(e);
	    timeServers.clear();

	    // parse, copy and reference whitespace-delimited timeServers
	    std::istringstream timeServersStream(timeServers_);
	    size_t index {0};
	    while (true) {
		std::string timeServerString;
		timeServersStream >> timeServerString;
		if (!timeServersStream) break;
		char * timeServer {strdup(timeServerString.c_str())};
		timeServers.push_back(timeServer);
		ESP_LOGI(name, "server %d %s", index, timeServer);
		sntp_setservername(index++, timeServer);
	    }

	    // forget remaining (freed) references
	    while (index < SNTP_MAX_SERVERS) {
		ESP_LOGI(name, "server %d -", index);
		sntp_setservername(index++, nullptr);
	    }

	    // (re)set SNTP operating mode and (re)start
	    ESP_LOGI(name, "start");
	    sntp_setoperatingmode(SNTP_OPMODE_POLL);
	    sntp_init();
	})
{
    sntp_set_time_sync_notification_cb(time_sync_notification);
}

TimeUpdate::~TimeUpdate() {
    ESP_LOGI(name, "stop");
    sntp_stop();
    for (auto e: timeServers) free(e);
    timeServers.clear();
}
