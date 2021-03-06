#include <memory>

#include "Error.h"
#include "NVSKeyValueBroker.h"

NVSKeyValueBroker::NVSKeyValueBroker(char const * name_)
:
    KeyValueBroker	(name_),
    nvs			([this](){
	    nvs_handle result;
	    Error::throwIf(nvs_open(name, NVS_READWRITE, &result));
	    return result;
	}())
{}

/* virtual */ bool NVSKeyValueBroker::set(char const * key, char const * value) {
    // cache this
    if (KeyValueBroker::set(key, value)) {
	// store this
	nvs_set_str(nvs, key, value);
	nvs_commit(nvs);
	return true;
    }
    return false;
}

/* virtual */ bool NVSKeyValueBroker::get(char const * key, std::string & value) {
    // try our cache
    if (KeyValueBroker::get(key, value)) return true;
    try {
	size_t length;
	Error::throwIf(nvs_get_str(nvs, key, nullptr, &length));
	std::unique_ptr<char> copy(new char[length]);
	Error::throwIf(nvs_get_str(nvs, key, copy.get(), &length));
	// cache this
	KeyValueBroker::set(key, copy.get());
	value = copy.get();
    } catch (esp_err_t & e) {
	if (ESP_ERR_NVS_NOT_FOUND == e) return false;
	throw;
    }
    return true;
}

NVSKeyValueBroker::~NVSKeyValueBroker() {
    nvs_close(nvs);
}
