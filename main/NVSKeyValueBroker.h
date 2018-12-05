#ifndef NVSKeyValueBroker_h_
#define NVSKeyValueBroker_h_

#include <memory>

#include <nvs.h>

#include "KeyValueBroker.h"

class NVSKeyValueBroker : public KeyValueBroker {
private:
    nvs_handle		nvs;

protected:
    virtual void set(char const * key, char const * value);
    virtual bool get(char const * key, std::string & value);

public:
    NVSKeyValueBroker(char const * name);
    ~NVSKeyValueBroker();
};

#endif