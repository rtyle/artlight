#include <memory>

#include <esp_log.h>

#include "Preferences.h"

#include "percentDecode.h"

template<typename T = esp_err_t>
static inline T throwIf(T t) {if (t) throw t; return t;}

void x(char const * & c) {}

Preferences::Preferences(char const * html_, KeyValueBroker & keyValueBroker_)
:
    Httpd		("preferences", Httpd::Config()
			    .task_priority_(5)
			    .stack_size_(4096)
			),

    html		(html_),

    keyValueBroker	(keyValueBroker_),

    uri(*this, "/", HTTP_GET, [this](httpd_req_t * req){
	// publish all key, value pairs from URL query
	if (size_t size = httpd_req_get_url_query_len(req)) {
	    std::unique_ptr<char> query(new char[++size]);
	    throwIf(httpd_req_get_url_query_str(req, query.get(), size));
	    ESP_LOGI(name, "%s", query.get());
	    for (char const * s = query.get(); *s;) {
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
	httpd_resp_send(req, html, strlen(html));
	return ESP_OK;
    }),

    dataUri(*this, "/data", HTTP_GET, [this](httpd_req_t * req){
	std::string keyValues = keyValueBroker.serialize();
	httpd_resp_send(req, keyValues.c_str(), keyValues.length());
	return ESP_OK;
    })
{}
