#include <esp_log.h>

#include "Timer.h"

#include "LuxTask.h"

static TickType_t soonAfterAvailable(unsigned available) {
    return 1 + available / portTICK_PERIOD_MS;
}

LuxTask::LuxTask(LuxSensor & luxSensor_)
:
    AsioTask {"luxTask", 5, 4096, 0},
    luxSensor {luxSensor_},
    // asio timers are not supported
    // adapt a FreeRTOS timer to post an update to this task.
    timer{name,
	soonAfterAvailable(luxSensor.tillAvailable()),
	false,
	[this]() {
	    io.post([this]() {
		this->update();
	    });
	}
    },
    lux{0.0f}
{}

void LuxTask::update() {
    // we try to skate on the edge (just under an overflow_error)
    try {
	lux = luxSensor.readLux();
	timer.setPeriod(soonAfterAvailable(luxSensor.increaseSensitivity()));
    } catch (std::underflow_error e) {
#if 0
	ESP_LOGE(name, "underflow");
#endif
	timer.setPeriod(soonAfterAvailable(luxSensor.increaseSensitivity()));
    } catch (std::overflow_error e) {
#if 0
	ESP_LOGE(name, "overflow");
#endif
	timer.setPeriod(soonAfterAvailable(luxSensor.decreaseSensitivity()));
    } catch (esp_err_t & e) {
	ESP_LOGE(name, "error %x", e);
    }
    timer.start();
}

/* virtual */ void LuxTask::run() {
    timer.start();

    // create some dummy work ...
    asio::io_service::work work(io);

    // ... so that we will run forever
    AsioTask::run();
}

float LuxTask::getLux() {return lux;}
