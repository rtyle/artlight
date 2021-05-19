#pragma once

#include "KeyValueBroker.h"

/// An MDNS instance is an RAII wrapper to the ESP-IDF mDNS API
/// (which communicates to the LWIP mDNS server).
class MDNS {
private:
    /// MDNS::Host constructor/destructor wraps mdns_init/mdns_free
    struct Host {
	Host();
	~Host();
    } host;

    std::string defaultHostname;
    KeyValueBroker::Observer const	hostnameObserver;

public:
    /// An MDNS::Service represents an mDNS service registration
    struct Service {
    private:
	char const *	type;
	char const *	protocol;
    public:
	Service(
	    char const *	instance,
	    char const *	type,
	    char const *	protocol,
	    uint16_t		port,
	    char const *	key = nullptr,
	    ...);		// value, ..., nullptr

	~Service();
    };

    /// Construct an mDNS service for hostname
    /// (default "esp_" + macSuffix(interface))
    /// and instance
    MDNS(
	KeyValueBroker &	keyValueBroker,
	esp_mac_type_t		interface	= ESP_MAC_WIFI_SOFTAP,
	char const *		hostname	= nullptr,
	char const *		instance	= nullptr);
};
