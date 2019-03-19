#include <memory>

#include <esp_log.h>

#include "Preferences.h"

#include "percentDecode.h"

esp_err_t Preferences::get(httpd_req_t * req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_send(req, html, strlen(html));
    return ESP_OK;
}

esp_err_t Preferences::post(httpd_req_t * req) {
    // publish all key, value pairs from form-urlencoded data in body
    if (size_t left = req->content_len) {
	char * end;
	std::unique_ptr<char> form(end = new char[left + 1]());
	while (left) {
	    int lengthOrError = httpd_req_recv(req, end, left);
	    if (0 > lengthOrError) {
		ESP_LOGE(name, "/ POST receive error %d", lengthOrError);
		break;
	    } else if (0 == lengthOrError) {
		ESP_LOGE(name, "/ POST receive truncated");
		break;
	    }
	    left -= lengthOrError;
	    end  += lengthOrError;
	}
	if (!left) {
	    *end = 0;
	    char const * s = form.get();
	    ESP_LOGI(name, "%s", s);
	    while (*s) {
		char * t = const_cast<char *>(s);
		char const * k = t; percentDecode(t, s, '=');
		if ('=' != *s++) break;
		*t++ = 0;
		char const * v = t; percentDecode(t, s, '&');
		if (*s && '&' != *s++) break;
		*t++ = 0;
		keyValueBroker.publish(k, v);
	    }
	}
    }
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_send(req, html, strlen(html));
    return ESP_OK;
}

Preferences::Preferences(
    char const *	html_,
    KeyValueBroker &	keyValueBroker_,
    char const *	favicon_,
    size_t		faviconSize_)
:
    Httpd		("preferences", Httpd::Config()
			    .task_priority_(5)
			    .stack_size_(4096)
			),

    html		(html_),

    keyValueBroker	(keyValueBroker_),

    favicon		(favicon_),
    faviconSize		(faviconSize_),

    mdnsService		(nullptr, "_http", "_tcp", 80),

    uri {
	{*this, "/",
	    HTTP_GET, [this](httpd_req_t * req) {return get(req);}},
	{*this, "/preferences",
	    HTTP_GET, [this](httpd_req_t * req) {return get(req);}},
    },

    uriPost {
	{*this, "/",
	    HTTP_POST, [this](httpd_req_t * req) {return post(req);}},
	{*this, "/preferences",
	    HTTP_POST, [this](httpd_req_t * req) {return post(req);}},
    },

    dataUri(*this, "/data", HTTP_GET, [this](httpd_req_t * req) {
	httpd_resp_set_type(req, "application/json");
	std::string keyValues = keyValueBroker.serialize();
	httpd_resp_send(req, keyValues.c_str(), keyValues.length());
	return ESP_OK;
    }),

    dataDefaultUri(*this, "/dataDefault", HTTP_GET, [this](httpd_req_t * req) {
	httpd_resp_set_type(req, "application/json");
	std::string keyValues = keyValueBroker.serializeDefault();
	httpd_resp_send(req, keyValues.c_str(), keyValues.length());
	return ESP_OK;
    }),

    faviconUri(*this, "/favicon.ico", HTTP_GET, [this](httpd_req_t * req) {
	httpd_resp_set_type(req, "image/x-icon");
	httpd_resp_send(req, favicon, faviconSize);
	return ESP_OK;
    })
{}
