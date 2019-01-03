#include <esp_log.h>

#include "Timer.h"

#include "LuxTask.h"

static TickType_t tickAfterIntegration(TSL2561 & tsl2561) {
    return 1 + tsl2561.getIntegrationTime() / portTICK_PERIOD_MS;
}

LuxTask::LuxTask(I2C::Master const * i2cMaster)
:
    AsioTask("luxTask", 5, 4096, 0),
    luxBy{},
    integrationTime(0),
    tsl2561(i2cMaster, TSL2561::I2cAddress::floating,
	static_cast<TSL2561::IntegrationTime>(integrationTime)),
    // asio timers are not supported
    // adapt a FreeRTOS timer to post an update to this task.
    timer(name,
	    tickAfterIntegration(tsl2561),
	    false,
	    [this]() {
		io.post([this]() {
		    this->update();
		});
	    }
	),
    lux{0.0f}
{}

void LuxTask::update() {
    try {
	// longer integration times and/or brighter light
	// risk overflowing (wrapping) the TSL2561's counters.
	// to address this, the brightest light should be measured with the
	// shortest integration time.
	// manually, using our timer (10ms tick resolution),
	// we can not do better than the automatic
	// TSL2561::IntegrationTime::fastest (13.7ms).
	// we cycle through all automatic integration times
	// and trust the largest result.
	luxBy[integrationTime] = tsl2561.getLux();
	if (sizeof luxBy / sizeof *luxBy == ++integrationTime) {
	    integrationTime = 0;
	    float luxMax = 0.0f;
	    for (auto e: luxBy) {if (luxMax < e) luxMax = e;}
//	    ESP_LOGI(name, "%f = max(%f %f %f)", luxMax, luxBy[0], luxBy[1], luxBy[2]);
	    lux = luxMax;
	}
	tsl2561.stop();
	tsl2561.setTiming(static_cast<TSL2561::IntegrationTime>(integrationTime));
	tsl2561.start();
	timer.setPeriod(tickAfterIntegration(tsl2561));
	timer.start();
    } catch (esp_err_t & e) {
	ESP_LOGE(name, "error %x", e);
    }
}

/* virtual */ void LuxTask::run() {
    timer.start();

    // create some dummy work ...
    asio::io_service::work work(io);

    // ... so that we will run forever
    AsioTask::run();
}

float LuxTask::getLux() {return lux;}
