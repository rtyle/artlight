#pragma once

#include <map>
#include <mutex>
#include <set>
#include <string>

class KeyValueBroker {
public:
    class Observer {
    public:
	using Observe = std::function<void(char const *)>;

	KeyValueBroker &	keyValueBroker;
	char const * const	key;
	char const * const	defaultValue;
	Observe const		observe;

	Observer(
	    KeyValueBroker &	keyValueBroker,
	    char const *	key,
	    char const *	defaultValue,
	    Observe &&		observe);

	void operator()(char const * value) const;

	~Observer();
    };
    friend class Observer;

    class GeneralObserver {
    public:
	using Observe = std::function<void(char const *, char const *, bool)>;

	KeyValueBroker &	keyValueBroker;
	Observe const		observe;

	GeneralObserver(
	    KeyValueBroker &	keyValueBroker,
	    Observe &&		observe);

	void operator()(char const *, char const *, bool = false) const;

	~GeneralObserver();
    };
    friend class Observer;

    KeyValueBroker(char const * name);

    virtual ~KeyValueBroker();

    void publish(
	char const *	key,
	char const *	value,
	bool		fromPeer = false);

    static std::string serialize(char const * key, char const * value);

    std::string serialize();
    std::string serializeDefault();

protected:
    char const * const name;

    // return true if new/different value was set
    virtual bool set(char const * key, char const * value);

    // return true (with value) if we could get it
    virtual bool get(char const * key, std::string & value);

private:
    std::recursive_mutex mutex;
    using Observers = std::set<Observer const *>;
    std::map<std::string, Observers *> observersFor;
    std::set<GeneralObserver const *> generalObservers;
    std::map<std::string, std::string> valueFor;
    std::map<std::string, std::string> defaultValueFor;

    void subscribe(Observer const & observer);
    void unsubscribe(Observer const & observer);
    void generalSubscribe(GeneralObserver const & generalObserver);
    void generalUnsubscribe(GeneralObserver const & generalObserver);
};
