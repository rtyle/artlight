#include <esp_log.h>

#include "MotionSensor.h"

MotionSensor::MotionSensor(char const * name_, asio::io_context & io_)
:
    name	{name_},
    io		{io_},
    motion	{0.0f},
    // asio timers are not supported
    // adapt a FreeRTOS timer to post an update
    timer{name,
	1,
	false,
	[this]() {
	    io.post([this]() {
		this->update();
	    });
	}
    }
{}

MotionSensor::~MotionSensor() {}

static TickType_t soonAfterAvailable(unsigned available) {
    return 2 + available / portTICK_PERIOD_MS;
}

void MotionSensor::update() {
    // we try to skate on the edge (just under an overflow_error)
    try {
	motion = readMotion();
	timer.setPeriod(soonAfterAvailable(increaseSensitivity()));
    } catch (std::underflow_error e) {
#if 0
	ESP_LOGE(name, "underflow %s", e.what());
#endif
	timer.setPeriod(soonAfterAvailable(increaseSensitivity()));
    } catch (std::overflow_error e) {
#if 0
	ESP_LOGE(name, "overflow %s", e.what());
#endif
	timer.setPeriod(soonAfterAvailable(decreaseSensitivity()));
    } catch (esp_err_t & e) {
	ESP_LOGE(name, "error %x", e);
    }
    timer.start();
}

float MotionSensor::getMotion() {return motion;}
