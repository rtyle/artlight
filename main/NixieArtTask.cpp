#include <random>
#include <sstream>

#include <esp_log.h>

#include "APA102.h"
#include "NixieArtTask.h"
#include "Timer.h"

extern "C" uint64_t get_time_since_boot();

using APA102::LED;
using LEDI = APA102::LED<int>;

static unsigned constexpr millisecondsPerSecond	{1000u};
static unsigned constexpr microsecondsPerSecond	{1000000u};

void NixieArtTask::update_() {
}

void NixieArtTask::update() {
    // if the rate at which we complete work
    // is less than the periodic demand for such work
    // the work queue will eventually exhaust all memory.
    if (updated++) {
	// catch up to the rate of demand
	ESP_LOGW(name, "update rate too fast. %u", updated - 1);
	return;
    }
    update_();
    // ignore any queued update work
    io.poll();
    updated = 0;
}

uint8_t spiMode[] = {
    APA102::spiMode,
    APA102::spiMode
};

NixieArtTask::NixieArtTask(
    SPI::Bus const		(&spiBus)[2],
    std::function<float()>	getLux_,
    KeyValueBroker &		keyValueBroker_)
:
    ArtTask		{"NixieArtTask", 5, 0x10000, 1,
    			spiBus, getLux_, keyValueBroker_, 512, spiMode},

    updated(0)
{}

void NixieArtTask::run() {
    // asio timers are not supported
    // adapt a FreeRTOS timer to post timeout to this task.
    Timer updateTimer(name, 8, true, [this](){
	io.post([this](){
	    this->update();
	});
    });

    updateTimer.start();

    // create some dummy work ...
    asio::io_service::work work(io);

    // ... so that we will run forever
    AsioTask::run();
}

NixieArtTask::~NixieArtTask() {
    stop();
    ESP_LOGI(name, "~NixieArtTask");
}
