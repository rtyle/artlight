#include <esp_log.h>

#include "Ring.h"
#include "Timer.h"

#include "function.h"

using APA102::LED;
using function::compose;
using function::combine;

namespace Ring {

static float const pi = std::acos(-1.0f);

template <typename T>
class Dim {
private:
    float	dim;
public:
    Dim(float dim_) : dim(dim_) {}
    T operator()(T t) {return t * dim;}
};

class Wave {
private:
    float	offset;
    float	frequency;
    float	speed;
public:
    Wave(
	float offset_		= 0.0f,
	float frequency_	= 1.0f,
	float speed_		= 1.0f)
    :
	offset		(offset_),
	frequency	(frequency_),
	speed		(speed_)
    {}
    float operator()(PT pt) {
	return (1.0f
	    + std::cos(2.0f * pi * frequency * (pt.p - offset - pt.t * speed)))
	/ 2.0f;
    }
};

static LED<> gamma(LED<> led) {return led.gamma();}

void ArtTask::update() {
    // dim factor is calculated as a function of the current ambient lux.
    // this will range from 3/16 to 16/16 with the numerator increasing by
    // 1 as the lux doubles up until 2^13 (~full daylight, indirect sun).
    // an LED value of 128 will be dimmed to 24 in complete darkness (lux 0)
    // which will be APA102 gamma corrected to 1.
    Dim<LED<>> dim((3.0f + std::min(13.0f, std::log2(1.0f + getLux()))) / 16.0f);

    static size_t constexpr circumference = 144;
    APA102::Message<circumference> message;

    float time = smoothTime.millisecondsSinceTwelveLocaltime() / 1000.0f;
    size_t place = 0;
    for (auto & e: message.encodings) {
	e = gamma(dim(art(PT(static_cast<float>(place) / circumference, time))));
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

ArtTask::ArtTask(
    SPI::Bus const *		spiBus1,
    SPI::Bus const *		spiBus2,
    std::function<float()>	getLux_,
    KeyValueBroker &		keyValueBroker_)
:
    ::ArtTask		("ringArtTask", 5, 16384, 1,
			spiBus1, spiBus2, getLux_, keyValueBroker_),

    art([this](PT pt)->LED<>{
	uint8_t x = 255 * ((1.0f + std::sin(2.0f * pi * (pt.p - pt.t))) / 2.0f);
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
