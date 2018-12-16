#include <sstream>
#include <string>

#include <esp_log.h>
#include <esp_https_ota.h>

#include "OtaTask.h"

// we should be able to use std::to_string but
// https://github.com/espressif/esp-idf/issues/1445
template <typename T>
static std::string to_string(T t) {
    std::ostringstream stream;
    stream << t;
    return stream.str();
}

template <typename T> T from_string(char const * s);

template <>
bool from_string(char const * s) {
    switch (*s) {
    case 0:
    case '0':
    case 'f':
    case 'F':
	return false;
    }
    return true;
}
template <typename T>
static T from_string(char const * s) {
    std::istringstream stream(s);
    T t;
    stream >> t;
    return t;
}

OtaTask::OtaTask(
    char const *	url_,
    char const *	certificate_,
    KeyValueBroker &	keyValueBroker_)
:
    AsioTask("otaTask", 5, 4096, 0),
    url				(url_),
    certificate			(certificate_),
    keyValueBroker		(keyValueBroker_),
    otaUrlObserver		(keyValueBroker, "otaUrl", url.c_str(),
	[this](char const * urlObserved){
	    std::string urlCopy(urlObserved);
	    io.post([this, urlCopy](){
		url = urlCopy;
	    });
	}),
    otaStartObserverEntered	(0),
    otaStartObserver		(keyValueBroker, "otaStart", "0",
	[this](char const * startObserved){
	    if (otaStartObserverEntered) return;
	    ++otaStartObserverEntered;
	    bool start = from_string<bool>(startObserved);
	    if (start) {
		// acknowledge start.
		// this will cause us to be reentered, which we guard against
		keyValueBroker.publish("otaStart", "0");
	    }
	    --otaStartObserverEntered;
	    io.post([this, start](){
		if (start) {
		    update();
		}
	    });
	})
{}

void OtaTask::update() {
    ESP_LOGI(name, "esp_https_ota %s", url.c_str());
    esp_http_client_config_t config {};
    config.url = url.c_str();
    config.cert_pem = certificate.c_str();
    if (ESP_OK == esp_https_ota(&config)) {
	ESP_LOGI(name, "restart");
	esp_restart();
    }
}

/* virtual */ void OtaTask::run() {
    asio::io_service::work work(io);
    AsioTask::run();
}

/* virtual */ OtaTask::~OtaTask() {
    stop();
}
