#include <esp_log.h>

#include "Ring.h"
#include "Timer.h"

#include "function.h"

using APA102::LED;
using function::compose;
using function::combine;

namespace Ring {

void ArtTask::update() {
    static size_t constexpr diameter = 144;
    APA102::Message<diameter> message;

    float time = esp_timer_get_time() / 1000000.0f;
    size_t place = 0;
    for (auto & e: message.encodings) {
	e = art(TP(time, static_cast<float>(place) / diameter));
	place++;
    }
    {
	SPI::Transaction transaction1(spiDevice1, SPI::Transaction::Config()
	    .tx_buffer_(&message)
	    .length_(message.length()));
	SPI::Transaction transaction2(spiDevice2, SPI::Transaction::Config()
	    .tx_buffer_(&message)
	    .length_(message.length()));
    }
}

static float pi = std::acos(-1);

ArtTask::ArtTask(
    SPI::Bus const *		spiBus1,
    SPI::Bus const *		spiBus2,
    std::function<float()>	getLux_,
    KeyValueBroker &		keyValueBroker_)
:
    ::ArtTask		("ringArtTask", 5, 16384, 1,
			spiBus1, spiBus2, getLux_, keyValueBroker_),

    art([](TP tp)->LED<>{
	uint8_t x = 255 * std::sin(2.0f * pi * (tp.p - tp.t));
	return LED<>(x, x, x);
    })
{}

/* virtual */ void ArtTask::run() {
    // asio timers are not supported
    // adapt a FreeRTOS timer to post timeout to this task.
    Timer updateTimer(name, 1, true, [this](){
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

/* virtual */ ArtTask::~ArtTask() {
    stop();
    ESP_LOGI(name, "~Ring::ArtTask");
}

}
