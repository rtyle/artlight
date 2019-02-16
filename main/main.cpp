#include <esp_log.h>

#include <nvs_flash.h>

#include "ArtTask.h"
#include "AsioTask.h"
#include "Event.h"
#include "I2C.h"
#include "LEDC.h"
#include "LuxTask.h"
#include "MDNS.h"
#include "NVSKeyValueBroker.h"
#include "OtaTask.h"
#include "Pin.h"
#include "ProvisionTask.h"
#include "Preferences.h"
#include "Qio.h"
#include "SPI.h"
#include "TimeUpdate.h"
#include "Wifi.h"

// ArtLightApplication_h must be defined as the include file that declares
// the DerivedArtTask class
#include ArtLightApplication_h

// COMPONENT_EMBED_FILES start
extern char const preferencesFavicon0[]
			asm("_binary_preferencesFavicon_ico_start");
extern unsigned char const provisionCert0[]
			asm("_binary_provision_ca_cert_pem_start");
extern unsigned char const provisionKey0[]
			asm("_binary_provision_ca_key_pem_start");
extern char const provisionResponseFavicon0[]
			asm("_binary_provisionResponseFavicon_start");

// COMPONENT_EMBED_FILES end (no null terminator added)
extern char const preferencesFavicon1[]
			asm("_binary_preferencesFavicon_ico_end");
extern unsigned char const provisionCert1[]
			asm("_binary_provision_ca_cert_pem_end");
extern unsigned char const provisionKey1[]
			asm("_binary_provision_ca_key_pem_end");
extern char const provisionResponseFavicon1[]
			asm("_binary_provisionResponseFavicon_end");

// COMPONENT_EMBED_TXTFILES (null terminator added)
extern char const otaCertificate[]
			asm("_binary_ota_ca_cert_pem_start");
extern char const preferencesHtml[]
			asm("_binary_preferences_html_start");

class Main : public AsioTask {
public:
    asio::io_context::work work;

    NVSKeyValueBroker keyValueBroker;

    class Disconnected {
    public:
	Main & main;
	ProvisionTask provisionTask;
	Disconnected(Main & main_)
	:
	    main(main_),
	    provisionTask(
		provisionCert0, provisionCert1 - provisionCert0,
		provisionKey0 , provisionKey1  - provisionKey0 ,
		provisionResponseFavicon0,
		    provisionResponseFavicon1 - provisionResponseFavicon0)
	{
	    provisionTask.start();
	}
	~Disconnected() {
	    ESP_LOGI(main.name, "~Disconnected");
	}
    };
    std::unique_ptr<Disconnected> disconnected;

    class Connected {
    public:
	Main & main;
	MDNS mdns;
	TimeUpdate timeUpdate;
	OtaTask otaTask;
	Preferences preferences;
	Connected(Main & main_)
	:
	    main(main_),
	    mdns(),
	    timeUpdate("timeUpdate", main.keyValueBroker),
	    otaTask(CONFIG_OTA_URL, otaCertificate, main.keyValueBroker),
	    preferences(preferencesHtml, main.keyValueBroker,
		preferencesFavicon0,
		    preferencesFavicon1 - preferencesFavicon0)
	{
	    otaTask.start();
	}
	~Connected() {
	    ESP_LOGI(main.name, "~Connected");
	}
    };
    std::unique_ptr<Connected> connected;

    Event::Observer staGotIpObserver;
    Event::Observer staDisconnectedObserver;

    Wifi wifi;

    I2C::Master const i2cMaster;
    LuxTask luxTask;

    SPI::Bus const spiBus1;
    SPI::Bus const spiBus2;

    LEDC::ISR		ledISR;
    LEDC::Timer		ledTimer;
    LEDC::Channel	ledChannel[3][3];

    ObservablePin::ISR		pinISR;
    ObservablePin::Task		pinTask;

    ObservablePin		aPin;
    ObservablePin		bPin;
    ObservablePin		cPin;
    ObservablePin		irPin;

    ObservablePin::Observer	aPinObserver;
    ObservablePin::Observer	bPinObserver;
    ObservablePin::Observer	cPinObserver;
    ObservablePin::Observer	irPinObserver;

    std::unique_ptr<ArtTask> artTask;

    Main()
    :
	AsioTask(),	// this task

	work(io),

	keyValueBroker("keyValueBroker"),

	disconnected(nullptr),
	connected(nullptr),

	staGotIpObserver(SYSTEM_EVENT_STA_GOT_IP,
		[this](system_event_t const * event)->esp_err_t{
	    // we will get here after every successful esp_wifi_connect attempt
	    // *and* whenever our IP address has changed (new DHCP lease).
	    io.post([this](){
		disconnected.reset(nullptr);
		// we must always reset our connected object
		connected.reset(new Connected(*this));
	    });
	    return ESP_OK;
	}),

	staDisconnectedObserver(SYSTEM_EVENT_STA_DISCONNECTED,
		[this](system_event_t const * event)->esp_err_t{
	    // we will get here after every failed esp_wifi_connect attempt
	    io.post([this](){
		connected.reset(nullptr);
		// no need to reset our disconnected object
		// if we already have one
		if (!disconnected) {
		    disconnected.reset(new Disconnected(*this));
		}
	    });
	    return ESP_OK;
	}),

	wifi("WIFI"),

	// internal pullups on silicon are rather high (~50k?)
	// external 4.7k is still too high. external 1k works
	i2cMaster(I2C::Config()
		.sda_io_num_(GPIO_NUM_21) //.sda_pullup_en_(GPIO_PULLUP_ENABLE)
		.scl_io_num_(GPIO_NUM_22) //.scl_pullup_en_(GPIO_PULLUP_ENABLE)
		.master_clk_speed_(400000),	// I2C fast mode
	    I2C_NUM_0, 0),
	luxTask(&i2cMaster),

	spiBus1(HSPI_HOST, SPI::Bus::Config()
		.mosi_io_num_(SPI::Bus::HspiConfig.mosi_io_num)
		.sclk_io_num_(SPI::Bus::HspiConfig.sclk_io_num),
	    1),
	spiBus2(VSPI_HOST, SPI::Bus::Config()
		.mosi_io_num_(SPI::Bus::VspiConfig.mosi_io_num)
		.sclk_io_num_(SPI::Bus::VspiConfig.sclk_io_num),
	    2),

	ledISR(),
	ledTimer(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_8_BIT, 10000),

	ledChannel {
	    {
		LEDC::Channel(ledTimer, GPIO_NUM_19, 0),
		LEDC::Channel(ledTimer, GPIO_NUM_16, 0),
		LEDC::Channel(ledTimer, GPIO_NUM_17, 0),
	    },
	    {
		LEDC::Channel(ledTimer, GPIO_NUM_4 , 0),
		LEDC::Channel(ledTimer, GPIO_NUM_25, 0),
		LEDC::Channel(ledTimer, GPIO_NUM_26, 0),
	    },
	    {
		LEDC::Channel(ledTimer, GPIO_NUM_33, 0),
		LEDC::Channel(ledTimer, GPIO_NUM_27, 0),
		LEDC::Channel(ledTimer, GPIO_NUM_12, 0),
	    },
	},

	pinISR(),
	pinTask("pinTask", 5, 4096, tskNO_AFFINITY, 128),

	aPin(GPIO_NUM_5 , GPIO_MODE_INPUT, GPIO_PULLUP_ENABLE,
	    GPIO_PULLDOWN_DISABLE, GPIO_INTR_ANYEDGE, pinTask),
	bPin(GPIO_NUM_36, GPIO_MODE_INPUT, GPIO_PULLUP_DISABLE,
	    GPIO_PULLDOWN_DISABLE, GPIO_INTR_ANYEDGE, pinTask),
	cPin(GPIO_NUM_15, GPIO_MODE_INPUT, GPIO_PULLUP_ENABLE,
	    GPIO_PULLDOWN_DISABLE, GPIO_INTR_ANYEDGE, pinTask),
	irPin(GPIO_NUM_34, GPIO_MODE_INPUT, GPIO_PULLUP_DISABLE,
	    GPIO_PULLDOWN_DISABLE, GPIO_INTR_ANYEDGE, pinTask),

	aPinObserver(aPin, [this](){
	    ESP_LOGI(name, "aPin %d", aPin.get_level());
	}),
	bPinObserver(bPin, [this](){
	    ESP_LOGI(name, "bPin %d", bPin.get_level());
	}),
	cPinObserver(cPin, [this](){
	    ESP_LOGI(name, "cPin %d", cPin.get_level());
	}),
	irPinObserver(irPin, [this](){
	    ESP_LOGI(name, "irPin %d", irPin.get_level());
	}),

	artTask(new DerivedArtTask(&spiBus1, &spiBus2,
	    [this](){return luxTask.getLux();},
	    keyValueBroker))
    {
	std::setlocale(LC_ALL, "en_US.utf8");

	// create LwIP task
	tcpip_adapter_init();

	wifi.start();

	disconnected.reset(new Disconnected(*this));

	luxTask.start();

	for (auto & rgb: ledChannel) {
	    for (auto & p: rgb) {
		p.set_fade_with_time(128, 1000);
		p.fade_start();
	    }
	}

	pinTask.start();

	artTask->start();
    }

    ~Main() {}
};

extern "C"
void app_main() {
    // initialize Non-Volatile Storage.
    // this is used to save
    //	* Wi-Fi provisioning information
    //	* Application preferences
    ESP_ERROR_CHECK(nvs_flash_init());

    Main().run();
}
