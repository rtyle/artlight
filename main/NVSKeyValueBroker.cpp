#include "NVSKeyValueBroker.h"

template<typename T>
static inline T throwIf(T t) {if (t) throw t; return t;}

NVSKeyValueBroker::NVSKeyValueBroker(char const * name_)
:
    KeyValueBroker	(name_),
    nvs			([this](){
	    nvs_handle result;
	    throwIf(nvs_open(name, NVS_READWRITE, &result));
	    return result;
	}())
{}

/* virtual */ void NVSKeyValueBroker::set(char const * key, char const * value) {
    nvs_set_str(nvs, key, value);
    nvs_commit(nvs);
}

/* virtual */ bool NVSKeyValueBroker::get(char const * key, std::string & value) {
    try {
	size_t length;
	throwIf(nvs_get_str(nvs, key, nullptr, &length));
	std::unique_ptr<char> copy(new char[length]);
	throwIf(nvs_get_str(nvs, key, copy.get(), &length));
	value = copy.get();
    } catch (esp_err_t & e) {
	if (ESP_ERR_NVS_NOT_FOUND) return false;
	throw;
    }
    return true;
}

NVSKeyValueBroker::~NVSKeyValueBroker() {
    nvs_close(nvs);
}
