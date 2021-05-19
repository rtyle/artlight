#pragma once

#include <string>

#include "nvs.h"

#include "KeyValueBroker.h"

class NVSKeyValueBroker : public KeyValueBroker {
private:
    nvs_handle		nvs;

protected:
    virtual bool set(char const * key, char const * value);
    virtual bool get(char const * key, std::string & value);

public:
    NVSKeyValueBroker(char const * name);
    ~NVSKeyValueBroker();
};
