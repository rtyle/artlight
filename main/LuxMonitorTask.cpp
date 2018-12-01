#include <esp_log.h>

#include "Timer.h"

#include "LuxMonitorTask.h"

LuxMonitorTask::LuxMonitorTask(I2C::Master const * i2cMaster)
:
    AsioTask("luxMonitorTask", 5, 4096, 0),
    tsl2561(i2cMaster, TSL2561::I2cAddress::floating),
    lux{0}
{}

void LuxMonitorTask::update() {
    try {
	lux = tsl2561.getLux();
    } catch (esp_err_t & e) {
	ESP_LOGE(name, "error %x", e);
    }
}

/* virtual */ void LuxMonitorTask::run() {
    // asio timers are not supported
    // adapt a FreeRTOS timer to post timeout to this task.
    Timer updateTimer(name,
	// the tick above the TSL2561 IntegrationTime
	1 + tsl2561.getIntegrationTime() / portTICK_PERIOD_MS,
	true,
	[this]() {
	    io.post([this]() {
		this->update();
	    });
	}
    );

    updateTimer.start();

    // create some dummy work ...
    asio::io_service::work work(io);

    // ... so that we will run forever
    AsioTask::run();
}

float LuxMonitorTask::getLux() {return lux;}
