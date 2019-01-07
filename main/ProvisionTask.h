#pragma once

#include <memory>

#include "sdkconfig.h"

#include <openssl/ssl.h>

#include <asio/ip/tcp.hpp>

#include "AsioTask.h"

/// A ProvisionTask is an AsioTask that implements an HTTPS server
/// in Wi-Fi Access Point (AP) mode
/// for the purposes of provisioning in Wi-Fi station (STA) mode.
/// Once the SSID and password of the target network have been provided,
/// the ProvisionTask will switch to station mode.
/// Successful transition to station mode should be detected
/// and the ProvisionTask should be told to stop.
/// This implementation predates esp-idf support for an esp_https_server.
class ProvisionTask : public AsioTask {
private:
    unsigned char const * const	cert;
    size_t const		certSize;
    unsigned char const * const	key;
    size_t const		keySize;
    char const * const		responseFavicon;
    size_t const		responseFaviconSize;
    std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)> ctx;
    asio::ip::tcp::acceptor	acceptor;
    asio::ip::tcp::socket	client;
    std::unique_ptr<SSL, decltype(&SSL_free)> ssl;
    std::unique_ptr<SSL, decltype(&SSL_shutdown)> sslShutdown;

    void respond(char const * response, size_t length);
    bool readRequest();
    void readClient();
    void acceptClient();
public:

    ProvisionTask(
	unsigned char const *	cert,
	size_t			certSize,
	unsigned char const *	key,
	size_t			keySize,
	char const *		responseFavicon = nullptr,
	size_t			responseFaviconSize = 0);

    /* virtual */ ~ProvisionTask();
};
