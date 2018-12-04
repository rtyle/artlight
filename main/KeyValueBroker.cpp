#include <esp_log.h>

#include "KeyValueBroker.h"

KeyValueBroker::KeyValueBroker(char const * name_)
:
    name	(name_),
    observersFor()
{}

KeyValueBroker::~KeyValueBroker() {}

void KeyValueBroker::publish(
    char const *	key,
    char const *	value,
    bool		set_)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (set_) {
	set(key, value);
    }
    auto observers = observersFor.find(key);
    if (observers != observersFor.end()) {
	for (auto observer: *observers->second) {
	    ESP_LOGI(name, "publish %s %s", observer->key, value);
	    (*observer)(value);
	}
    }
}

void KeyValueBroker::subscribe(Observer const & observer) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    bool haveValue;
    std::string value;
    if ((haveValue = get(observer.key, value))) {
	// observe the previously published/set value
	ESP_LOGI(name, "subscribe %s %s", observer.key, value.c_str());
	observer(value.c_str());
    }
    // remember this subscription
    auto observersIt = observersFor.find(observer.key);
    Observers * observers;
    if (observersIt != observersFor.end()) {
	observers = observersIt->second;
    } else {
	observersFor[observer.key] = observers = new Observers();
    }
    observers->insert(&observer);
    if (!haveValue && observer.value) {
	// publish (and set) the observer's value
	publish(observer.key, observer.value, true);
    }
}

void KeyValueBroker::unsubscribe(Observer const & observer) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    auto observersIt = observersFor.find(observer.key);
    if (observersIt != observersFor.end()) {
	Observers * observers = observersIt->second;
	auto observerIt = observers->find(&observer);
	if (observerIt != observers->end()) {
	    observers->erase(observerIt);
	    if (!observers->size()) {
		observersFor.erase(observer.key);
		delete observers;
	    }
	}
    }
}

KeyValueBroker::Observer::Observer(
    KeyValueBroker &	keyValueBroker_,
    char const *	key_,
    char const *	value_,
    Observe &&		observe_)
:
    keyValueBroker	(keyValueBroker_),
    key			(key_),
    value		(value_),
    observe		(std::move(observe_))
{
    keyValueBroker.subscribe(*this);
}

KeyValueBroker::Observer::~Observer() {
    keyValueBroker.unsubscribe(*this);
}

void KeyValueBroker::Observer::operator() (char const * value) const {
    observe(value);
}
