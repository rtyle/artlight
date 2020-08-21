#include <esp_log.h>

#include "MotionSensor.h"

MotionSensor::MotionSensor(char const * name_, asio::io_context & io_)
:
    name	{name_},
    io		{io_},
    motion	{false},
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

void MotionSensor::setDuration(unsigned value) {
    if (value) {
	if (!timer.isActive()) {
	    timer.start();
	}
    } else {
	motion = true;
	timer.stop();
    }
}

void MotionSensor::update() {
    try {
	motion = readMotion();
    } catch (esp_err_t & e) {
	ESP_LOGE(name, "%s (0x%x)", esp_err_to_name(e), e);
    } catch (...) {
	ESP_LOGE(name, "unknown error");
    }
    timer.setPeriod(getPeriod());
    timer.start();
}

bool MotionSensor::getMotion() {return motion;}
