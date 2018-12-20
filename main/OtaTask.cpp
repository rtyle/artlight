#include <string>

#include <esp_log.h>
#include <esp_https_ota.h>

#include "OtaTask.h"

#include "fromString.h"

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
	    bool start = fromString<bool>(startObserved);
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
