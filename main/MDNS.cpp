#include <cstdarg>
#include <iomanip>
#include <sstream>

#include <esp_log.h>
#include <esp_system.h>
#include <mdns.h>

#include "MDNS.h"

MDNS::Service::Service(
    char const *	instance,
    char const *	type_,
    char const *	protocol_,
    uint16_t		port,
    char const *	key,
    ...)
:
    type	(type_),
    protocol	(protocol_)
{
    ESP_ERROR_CHECK(
	mdns_service_add(instance, type, protocol, port, nullptr, 0));
    va_list args;
    va_start(args, key);
    while (key) {
	char const * value = va_arg(args, char const *);
	ESP_ERROR_CHECK(
	    mdns_service_txt_item_set(type, protocol, key, value));
	key = va_arg(args, char const *);
    }
    va_end(args);
}

MDNS::Service::~Service() {
    ESP_ERROR_CHECK(mdns_service_remove(type, protocol));
}

MDNS::Host::Host() {
    ESP_ERROR_CHECK(mdns_init());
}

MDNS::Host::~Host() {
    mdns_free();
}

MDNS::MDNS(
    KeyValueBroker &	keyValueBroker,
    esp_mac_type_t	interface,
    char const *	hostname,
    char const *	instance)
:
    host(),
    defaultHostname([this, interface](){
	static unsigned constexpr interfaces = 0
	    | 1 << ESP_MAC_WIFI_STA
	    | 1 << ESP_MAC_WIFI_SOFTAP
	    | 1 << ESP_MAC_BT
	    | 1 << ESP_MAC_ETH
	    ;
	std::ostringstream os;
	os << "esp";
	if ((1 << interface) & interfaces) {
	    os << "_";
	    union {
		uint8_t whole[6];
		struct {
		    uint8_t prefix[3];
		    uint8_t suffix[3];
		};
	    } mac;
	    ESP_ERROR_CHECK(esp_read_mac(mac.whole, interface));
	    os << std::hex << std::setfill('0');
	    for (auto e: mac.suffix) os << std::setw(2) << static_cast<int>(e);
	}
	return os.str();
    }()),
    hostnameObserver {keyValueBroker, "_hostname",
	hostname ? hostname : defaultHostname.c_str(),
	[this](char const * hostname){
	    ESP_LOGI("mDNS", "hostname %s", hostname);
	    ESP_ERROR_CHECK(mdns_hostname_set(hostname));
	}
    }
{
    if (instance) {
	ESP_LOGI("mDNS", "instance %s", instance);
	ESP_ERROR_CHECK(mdns_instance_name_set(instance));
    }
}
