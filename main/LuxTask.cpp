#include <esp_log.h>

#include "Timer.h"

#include "LuxTask.h"

LuxTask::LuxTask(I2C::Master const * i2cMaster)
:
    AsioTask("luxTask", 5, 4096, 0),
    tsl2561(i2cMaster, TSL2561::I2cAddress::floating,
	// using the fastest integration time
	// * shortens our reaction time to changing brightness
	// * lessens the chance for TSL2561 overflow
	// * reduces accuracy somewhat measuring a constant brightness
	TSL2561::IntegrationTime::fastest),
    lux{0}
{}

void LuxTask::update() {
    try {
	lux = tsl2561.getLux();
//	ESP_LOGI(name, "lux %f", static_cast<float>(lux));
    } catch (esp_err_t & e) {
	ESP_LOGE(name, "error %x", e);
    }
}

/* virtual */ void LuxTask::run() {
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

float LuxTask::getLux() {return lux;}
