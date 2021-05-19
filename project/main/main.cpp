#include "esp_log.h"

#include "nvs_flash.h"

#include "AsioTask.h"
#include "Event.h"
#include "I2C.h"
#include "MDNS.h"
#include "NVSKeyValueBroker.h"
#include "OtaTask.h"
#include "PeerTask.h"
#include "ProvisionTask.h"
#include "Preferences.h"
#include "Qio.h"
#include "SPI.h"
#include "TimeUpdate.h"
#include "WebSocketTask.h"
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
		provisionResponseFavicon1 - provisionResponseFavicon0,
		main.keyValueBroker)
	{
	    ESP_LOGI(main.name, "Disconnected");
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
	PeerTask peerTask;
	WebSocketTask webSocketTask;
	Connected(Main & main_)
	:
	    main(main_),
	    mdns(main.keyValueBroker),
	    timeUpdate("timeUpdate", main.keyValueBroker),
	    otaTask(CONFIG_OTA_URL, otaCertificate, main.keyValueBroker),
	    preferences(preferencesHtml, main.keyValueBroker,
		preferencesFavicon0,
		preferencesFavicon1 - preferencesFavicon0),
	    peerTask(main.keyValueBroker),
	    webSocketTask(main.keyValueBroker)
	{
	    ESP_LOGI(main.name, "Connected");
	    otaTask.start();
	    peerTask.start();
	    webSocketTask.start();
	}
	~Connected() {
	    ESP_LOGI(main.name, "~Connected");
	}
    };
    std::unique_ptr<Connected> connected;

    Event::Loop eventLoopDefault;
    Event::Handler staGotIpHandler;
    Event::Handler staDisconnectedHandler;

    Wifi wifi;

    DerivedArtTask artTask;

    Main()
    :
	AsioTask{},	// this task

	work{io},

	keyValueBroker{"keyValueBroker"},

	disconnected{nullptr},
	connected{nullptr},

	eventLoopDefault{},

	staGotIpHandler{nullptr, IP_EVENT, IP_EVENT_STA_GOT_IP,
		[this](esp_event_base_t, int32_t, void *){
	    // we will get here after every successful esp_wifi_connect attempt
	    // *and* whenever our IP address has changed (new DHCP lease).
	    io.post([this](){
		disconnected.reset(nullptr);
		// we must always reset our connected object
		connected.reset(new Connected(*this));
	    });
	}},

	staDisconnectedHandler{nullptr, WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,
		[this](esp_event_base_t, int32_t, void *){
	    // we will get here after every failed esp_wifi_connect attempt
	    io.post([this](){
		connected.reset(nullptr);
		// no need to reset our disconnected object
		// if we already have one
		if (!disconnected) {
		    disconnected.reset(new Disconnected(*this));
		}
	    });
	}},

	wifi{"WIFI"},

	artTask{keyValueBroker}
    {
	std::setlocale(LC_ALL, "en_US.utf8");

	// create LwIP task
	esp_netif_init();

	wifi.start();

	disconnected.reset(new Disconnected(*this));

	artTask.start();
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
