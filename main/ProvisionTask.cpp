#include <cstring>
#include <memory>
#include <string>

#include <esp_err.h>
#include <esp_log.h>
#include <esp_wifi.h>

#include "ProvisionTask.h"

#include "percentDecode.h"

// WARNING:
// in order for HTTP/1.1 persistent connections to work,
// the client must be able to tell where each response ends
// and the next one begins.
// to do this, the value of the
// Content-Length header HTTP Header Field
// must reflect the number of octets encoded in the body
// (encoded as raw string literals, below).
// in C++ translation phase 1,
// end-of-line indicators are replaced by newline characters.
// if, to start with, the end-of-line indicators are already
// newlines and a copy/paste operation to a command shell
// will preserve these and all other raw encodings,
// then its length can be measured by presenting it on the
// standard input of a "wc --bytes" command.
// be sure to measure everything between the raw character
// string literal delimiters, including the last newline.

static char response[] =
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/html; charset=utf-8\r\n"
"Content-Length: 707\r\n"
"\r\n" R"----(<!DOCTYPE html>
<html>
<head>
</head>
<body>
<form method='post'>
<div>
<label for='ssid'>SSID</label>
<input value='' type='text' id='ssid' name='ssid' minlength='1' maxlength='32' placeholder='1 to 32 characters'/>
</div>
<div>
<label for='password'>Password</label>
<input value='' type='password' id='password' name='password' minlength='8' maxlength='64' placeholder='8 to 64 characters'/>
</div>
<div>
<label for='hostname'>mDNS Hostname</label>
<input value='' type='text' id='hostname' name='_hostname' minlength='0' maxlength='64' placeholder='0 to 64 characters'/>
leave blank to accept default or previously defined preference.
</div>
<div>
<button>Submit</button>
</div>
</form>
</body>
</html>
)----";

static char responseNotFound[] =
"HTTP/1.1 404 Not Found\r\n"
"Content-Length: 0\r\n"
"\r\n";

static char responseMethodNotAllowed[] =
"HTTP/1.1 405 Method Not Allowed\r\n"
"Content-Length: 0\r\n"
"\r\n";

void ProvisionTask::respond(
    char const *	response,
    size_t		length)
{
    ESP_LOG_BUFFER_HEXDUMP(name, response, length, ESP_LOG_INFO);
    ssize_t errorOrLength = SSL_write(ssl.get(), response, length);
    switch (int error = SSL_get_error(ssl.get(), errorOrLength)) {
    case SSL_ERROR_SYSCALL:
	ESP_LOGE(name, "SSL_write system error %d (%s)",
	    errno, std::strerror(errno));
	break;
    case SSL_ERROR_SSL:
	ESP_LOGE(name, "SSL_write SSL_ERROR_SSL");
	break;
    case SSL_ERROR_NONE:
	if (errorOrLength != length) {
	    ESP_LOGE(name, "SSL_write short %zu < %zu",
		static_cast<size_t>(errorOrLength), length);
	}
	break;
    default:
	ESP_LOGE(name, "SSL_read unexpected error %d", error);
	break;
    }
}

bool ProvisionTask::readRequest() {
    ESP_LOGI(name, "readRequest");
    char request[2048];
    ssize_t readErrorOrLength
	= SSL_read(ssl.get(), request, sizeof request - 1);
    switch (int error = SSL_get_error(ssl.get(), readErrorOrLength)) {
    case SSL_ERROR_ZERO_RETURN:
	ESP_LOGI(name, "readRequest SSL_read zero");
	break;
    case SSL_ERROR_SYSCALL:
	ESP_LOGE(name, "readRequest SSL_read system error %d (%s)",
	    errno, std::strerror(errno));
	break;
    case SSL_ERROR_SSL:
	ESP_LOGE(name, "readRequest SSL_read SSL_ERROR_SSL");
	break;
    case SSL_ERROR_NONE:
	{
	    ESP_LOGI(name, "readRequest SSL_read %d octets", readErrorOrLength);
	    // request may have password in it!
	    // dump(name, "buffer", request, readErrorOrLength);
	    request[readErrorOrLength] = 0;
	    char const * s = request;
	    static char constexpr GET[] = "GET ";
	    static char constexpr POST[] = "POST ";
	    if (0 == strncmp(s, GET, sizeof GET - 1)) {
		// GET *
		s += sizeof GET - 1;
		if (*s == '/') {
		    ++s;
		    switch (*s++) {
		    case ' ':
			// GET /
			ESP_LOGI(name, "readRequest GET /");
			respond(response, sizeof response - 1);
			break;
		    case 'f':
			{
			    static char constexpr favicon[] = "favicon.ico ";
			    if (responseFavicon && responseFaviconSize &&
				    0 == strncmp(s - 1, favicon, sizeof favicon - 1)) {
				// GET /favicon.ico
				// if we have an untrusted (e.g. self-signed)
				// certificate browsers may ignore this
				// and use some kind of frightening alert icon
				// for us instead.
				ESP_LOGI(name, "readRequest GET /favicon.ico");
				respond(responseFavicon, responseFaviconSize);
				break;
			    } // else, fall through ...
			}
		    default:
			// GET /*
			ESP_LOGE(name, "readRequest GET /*");
			respond(responseNotFound, sizeof responseNotFound - 1);
			break;
		    }
		} else {
		    // GET *
		    ESP_LOGE(name, "readRequest GET *");
		    respond(responseNotFound, sizeof responseNotFound - 1);
		}
	    } else if (0 == strncmp(s, POST, sizeof POST - 1)) {
		// POST *
		s += sizeof POST - 1;
		if (*s == '/') {
		    ++s;
		    switch (*s++) {
		    case ' ': {
			// POST /
			ESP_LOGI(name, "readRequest POST /");
			wifi_config_t wifi_config;
			ESP_ERROR_CHECK(esp_wifi_get_config(
			    ESP_IF_WIFI_STA, &wifi_config));
			char * t;
			size_t z;

			static char constexpr CRLFCRLF[] = "\r\n\r\n";
			if ((s = std::strstr(s, CRLFCRLF))) {
			    s += sizeof CRLFCRLF - 1;
			    static char constexpr ssid[] = "ssid=";
			    if (0 == strncmp(s, ssid, sizeof ssid - 1)) {
				s += sizeof ssid - 1;

				t = reinterpret_cast<char *>
				    (wifi_config.sta.ssid);
				z = sizeof wifi_config.sta.ssid;
				memset(t, 0, z);
				percentDecode(t, s, '&', z);

				++s;

				static char constexpr pw[] = "password=";
				if (0 == strncmp(s, pw, sizeof pw - 1)) {
				    s += sizeof pw - 1;

				    t = reinterpret_cast<char *>
					(wifi_config.sta.password);
				    z = sizeof wifi_config.sta.password;
				    memset(t, 0, z);
				    percentDecode(t, s, '&', z);

				    ESP_LOGI(name, "readRequest reconnect");
				    ESP_ERROR_CHECK(esp_wifi_set_config(
					ESP_IF_WIFI_STA, &wifi_config));
				    ESP_ERROR_CHECK(esp_wifi_connect());

				    ++s;
				    static char constexpr hn[] = "_hostname=";
				    if (0 == strncmp(s, hn, sizeof hn - 1)) {
					s += sizeof hn - 1;

					char hostname[64 + 1] = {};
					t = hostname;
					z = sizeof hostname - 1;
					percentDecode(t, s, '&', z);

					if (*hostname) {
					    ESP_LOGI(name,
						"publish %s%s", hn, hostname);
					    keyValueBroker.publish(hn, hostname);
					}
				    }
				}
			    }
			}
			respond(response, sizeof response - 1);
			break;}
		    default:
			// POST /*
			ESP_LOGE(name, "readRequest POST /*");
			respond(responseNotFound, sizeof responseNotFound - 1);
			break;
		    }
		} else {
		    // POST *
		    ESP_LOGE(name, "readRequest POST *");
		    respond(responseNotFound, sizeof responseNotFound - 1);
		}
	    } else {
		// *
		ESP_LOGE(name, "readRequest *");
		respond(responseMethodNotAllowed, sizeof responseMethodNotAllowed - 1);
	    }
	    return true;
	}
	break;
    default:
	ESP_LOGE(name, "SSL_read unexpected error %d", error);
	break;
    }
    return false;
}

void ProvisionTask::readClient() {
    ESP_LOGI(name, "readClient");
    client.async_wait(asio::ip::tcp::socket::wait_type::wait_read, [this](
	    std::error_code const & ec){
	if (ec) {
	    ESP_LOGE(name, "wait read error %d (%s)",
		ec.value(), ec.message().c_str());
	    acceptClient();
	    return;
	}

	bool response;
	while ((response = readRequest()) && SSL_pending(ssl.get()));

	if (response) {
	    readClient();
	} else {
	    acceptClient();
	}
    });
}

void ProvisionTask::acceptClient() {
    ESP_LOGI(name, "acceptClient");
    acceptor.async_accept([this](
	    std::error_code const & ec, asio::ip::tcp::socket client){
	if (ec) {
	    ESP_LOGE(name, "accept error %d (%s)",
		ec.value(), ec.message().c_str());
	    acceptClient();
	    return;
	}

	ESP_LOGI(name, "accept %d", client.native_handle());

	std::unique_ptr<SSL, decltype(&SSL_free)>
	    ssl(SSL_new(ctx.get()), &SSL_free);
	if (!ssl) {
	    ESP_LOGE(name, "SSL_new failed");
	    acceptClient();
	    return;
	}

	SSL_set_fd(ssl.get(), client.native_handle());

	int acceptResult = SSL_accept(ssl.get());
	switch (int error = SSL_get_error(ssl.get(), acceptResult)) {
	case SSL_ERROR_NONE:
	    {
		ESP_LOGI(name, "SSL_accept");

		std::unique_ptr<SSL, decltype(&SSL_shutdown)>
		    sslShutdown(ssl.get(), &SSL_shutdown);

		// transfer ownership to this
		this->sslShutdown	= std::move(sslShutdown);
		this->ssl		= std::move(ssl);
		this->client		= std::move(client);

		readClient();
		return;
	    }
	    break;

	default:
	    ESP_LOGE(name, "SSL_accept unexpected error %d", error);
	}

	acceptClient();
	return;
    });
}

ProvisionTask::ProvisionTask(
    unsigned char const *	cert_,
    size_t			certSize_,
    unsigned char const *	key_,
    size_t			keySize_,
    char const *		responseFavicon_,
    size_t			responseFaviconSize_,
    KeyValueBroker &		keyValueBroker_)
:
    AsioTask("provisionTask", 5, 8192, 0),
    cert(cert_),
    certSize(certSize_),
    key(key_),
    keySize(keySize_),
    responseFavicon(responseFavicon_),
    responseFaviconSize(responseFaviconSize_),
    keyValueBroker(keyValueBroker_),
    ctx(SSL_CTX_new(TLS_server_method()), &SSL_CTX_free),
    acceptor(io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 443)),
    client(io),
    ssl(nullptr, &SSL_free),
    sslShutdown(nullptr, &SSL_shutdown)
{
    ESP_LOGI(name, "ProvisionTask::ProvisionTask");
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    if (!ctx) {
	ESP_LOGE(name, "SSL_CTX_new failed");
	abort();
    }

    if (!SSL_CTX_use_certificate_ASN1(ctx.get(), certSize, cert)) {
	ESP_LOGE(name, "SSL_CTX_use_certificate_ASN1 failed");
	abort();
    }

    if (!SSL_CTX_use_PrivateKey_ASN1(0, ctx.get(), key, keySize)) {
	ESP_LOGE(name, "SSL_CTX_use_PrivateKey_ASN1 failed");
	abort();
    }

    ESP_LOGI(name, "listen %d", acceptor.local_endpoint().port());
    acceptClient();
}

ProvisionTask::~ProvisionTask() {
    stop();
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_LOGI(name, "ProvisionTask::~ProvisionTask");
}
