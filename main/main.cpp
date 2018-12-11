#include <esp_log.h>

#include <nvs_flash.h>

#include "AsioTask.h"
#include "ClockArtTask.h"
#include "Event.h"
#include "I2C.h"
#include "LuxMonitorTask.h"
#include "NVSKeyValueBroker.h"
#include "OtaTask.h"
#include "ProvisionTask.h"
#include "Preferences.h"
#include "SPI.h"
#include "TimeUpdate.h"
#include "Wifi.h"

// COMPONENT_EMBED_FILES start
extern unsigned char const provisionCert0[]
			asm("_binary_provision_ca_cert_pem_start");
extern unsigned char const provisionKey0[]
			asm("_binary_provision_ca_key_pem_start");
extern char const provisionResponseFavicon0[]
			asm("_binary_provisionResponseFavicon_start");

// COMPONENT_EMBED_FILES end (no null terminator added)
extern unsigned char const provisionCert1[]
			asm("_binary_provision_ca_cert_pem_end");
extern unsigned char const provisionKey1[]
			asm("_binary_provision_ca_key_pem_end");
extern char const provisionResponseFavicon1[]
			asm("_binary_provisionResponseFavicon_end");

// COMPONENT_EMBED_TXTFILES (null terminator added)
extern char const otaCert[]
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
	TimeUpdate timeUpdate;
	OtaTask otaTask;
	Preferences preferences;
	Connected(Main & main_)
	:
	    main(main_),
	    timeUpdate("timeUpdate", main.keyValueBroker),
	    otaTask(CONFIG_OTA_URL, otaCert, CONFIG_OTA_RETRY),
	    preferences(preferencesHtml, main.keyValueBroker)
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
    LuxMonitorTask luxMonitorTask;

    SPI::Bus const spiBus1;
    SPI::Bus const spiBus2;
    ClockArtTask clockArtTask;

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
	luxMonitorTask(&i2cMaster),

	spiBus1(HSPI_HOST, SPI::Bus::Config()
		.mosi_io_num_(SPI::Bus::HspiConfig.mosi_io_num)
		.sclk_io_num_(SPI::Bus::HspiConfig.sclk_io_num),
	    1),
	spiBus2(VSPI_HOST, SPI::Bus::Config()
		.mosi_io_num_(SPI::Bus::VspiConfig.mosi_io_num)
		.sclk_io_num_(SPI::Bus::VspiConfig.sclk_io_num),
	    2),
	clockArtTask(&spiBus1, &spiBus2,
	    [this](){return luxMonitorTask.getLux();},
	    keyValueBroker)
    {
	std::setlocale(LC_ALL, "en_US.utf8");

	// create LwIP task
	tcpip_adapter_init();

	wifi.start();

	disconnected.reset(new Disconnected(*this));

	luxMonitorTask.start();

	clockArtTask.start();
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
