#include "sdkconfig.h"

#include <esp_log.h>
#include <esp_https_ota.h>

#include "OtaTask.h"
#include "Timer.h"

OtaTask::OtaTask(
    char const *	url_,
    char const *	cert_,
    TickType_t		retry_)
:
    AsioTask("otaTask", 5, 4096, 0),
    url(url_),
    cert(cert_),
    retry(retry_)
{}

void OtaTask::update() {
    ESP_LOGI(name, "esp_https_ota %s", url);
    esp_http_client_config_t config {};
    config.url = url;
    config.cert_pem = cert;
    if (ESP_OK == esp_https_ota(&config)) {
	ESP_LOGI(name, "restart");
	esp_restart();
    }
}

/* virtual */ void OtaTask::run() {
    // asio timers are not supported
    // adapt a FreeRTOS timer to post timeout to this task.
    Timer updateTimer(name, pdMS_TO_TICKS(retry * 60 * 1000), true, [this](){
	io.post([this](){
	    this->update();
	});
    });
    updateTimer.start();
    asio::io_service::work work(io);

    AsioTask::run();
}

/* virtual */ OtaTask::~OtaTask() {
    stop();
}
