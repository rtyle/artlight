#include <iomanip>
#include <sstream>

#include <esp_log.h>

#include "KeyValueBroker.h"

KeyValueBroker::KeyValueBroker(char const * name_)
:
    name	(name_),
    observersFor(),
    valueFor	()
{}

KeyValueBroker::~KeyValueBroker() {}

/* virtual */ bool KeyValueBroker::set(char const * key, char const * value) {
    auto valueIt = valueFor.find(key);
    if (valueIt != valueFor.end() && 0 == valueIt->second.compare(value)) {
	return false;
    }
    valueFor[key] = value;
    return true;
}

/* virtual */ bool KeyValueBroker::get(char const * key, std::string & value) {
    auto valueIt = valueFor.find(key);
    if (valueIt == valueFor.end()) return false;
    value = valueIt->second;
    return true;
}

std::ostream & operator<<(std::ostream & stream, std::string & value) {
    // https://en.cppreference.com/w/cpp/language/escape
    // https://tools.ietf.org/html/rfc7159
    // warning: this may not work well with non-ascii UTF8 encodings
    for (auto c: value) {
	if (std::iscntrl(c)) {
	    switch (c) {
	    case '\b':
		stream << "\\b"; break;
	    case '\f':
		stream << "\\f"; break;
	    case '\n':
		stream << "\\n"; break;
	    case '\r':
		stream << "\\r"; break;
	    case '\t':
		stream << "\\t"; break;
	    default:
		stream << "\\u"
		    << std::hex
		    << std::setw(4)
		    << std::setfill('0')
		    << c;
	    }
	} else {
	    switch (c) {
	    case '\\':
	    case '"':
		stream << '\\';
		// no break
	    default:
		stream << c;
	    }
	}
    }
    return stream;
}

static std::string serialize(std::map<std::string, std::string> const & map) {
    std::ostringstream stream;
    size_t count = 0;
    stream << '{';
    for (auto &e: map) {
	if (count++) {
	    stream << ',';
	}
	stream
	    << '"'
	    << e.first
	    << R"----(":")----"
	    << e.second
	    << '"';
    }
    stream << '}';
    return stream.str();
}

std::string KeyValueBroker::serialize() {
    return ::serialize(valueFor);
}

std::string KeyValueBroker::serializeDefault() {
    return ::serialize(defaultValueFor);
}

void KeyValueBroker::publish(
    char const *	key,
    char const *	value)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (set(key, value)) {
	auto observers = observersFor.find(key);
	if (observers != observersFor.end()) {
	    for (auto observer: *observers->second) {
		ESP_LOGI(name, "publish %s %s", observer->key, value);
		(*observer)(value);
	    }
	}
    }
}

void KeyValueBroker::subscribe(Observer const & observer) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    bool haveValue;
    std::string value;
    if ((haveValue = get(observer.key, value))) {
	// observe the previously published value
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
    if (observer.defaultValue) {
	defaultValueFor[observer.key] = observer.defaultValue;
	if (!haveValue) {
	    // publish the observer's defaultValue
	    publish(observer.key, observer.defaultValue);
	}
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
    char const *	defaultValue_,
    Observe &&		observe_)
:
    keyValueBroker	(keyValueBroker_),
    key			(key_),
    defaultValue	(defaultValue_),
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
