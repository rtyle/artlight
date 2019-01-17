#pragma once

/// An MDNS instance is an RAII wrapper to the ESP-IDF mDNS API
/// (which communicates to the LWIP mDNS server).
class MDNS {
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
	esp_mac_type_t	interface	= ESP_MAC_WIFI_STA,
	char const *	hostname	= nullptr,
	char const *	instance	= nullptr);

    ~MDNS();
};
