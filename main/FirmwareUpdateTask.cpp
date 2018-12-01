#include "sdkconfig.h"

#include <esp_log.h>
#include <esp_https_ota.h>

#include "FirmwareUpdateTask.h"
#include "Timer.h"

FirmwareUpdateTask::FirmwareUpdateTask(
    char const *	url_,
    char const *	cert_,
    TickType_t		pause_)
:
    AsioTask("firmwareUpdateTask", 5, 4096, 0),
    url(url_),
    cert(cert_),
    pause(pause_)
{}

void FirmwareUpdateTask::update() {
    ESP_LOGI(name, "esp_https_ota %s", url);
    esp_http_client_config_t config {};
    config.url = url;
    config.cert_pem = cert;
    if (ESP_OK == esp_https_ota(&config)) {
	ESP_LOGI(name, "restart");
	esp_restart();
    }
}

/* virtual */ void FirmwareUpdateTask::run() {
    // asio timers are not supported
    // adapt a FreeRTOS timer to post timeout to this task.
    Timer updateTimer(name, pause, true, [this](){
	io.post([this](){
	    this->update();
	});
    });
    updateTimer.start();
    asio::io_service::work work(io);

    AsioTask::run();
}

/* virtual */ FirmwareUpdateTask::~FirmwareUpdateTask() {
    stop();
}
