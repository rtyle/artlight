#ifndef KeyValueBroker_h_
#define KeyValueBroker_h_

#include <map>
#include <mutex>
#include <set>
#include <string>

class KeyValueBroker {
public:
    class Observer {
    public:
	typedef std::function<void(char const *)> Observe;

	KeyValueBroker &	keyValueBroker;
	char const * const	key;
	char const * const	value;
	Observe const		observe;

	Observer(
	    KeyValueBroker &	keyValueBroker,
	    char const *	key,
	    char const *	value,
	    Observe &&		observe);

	void operator()(char const * value) const;

	~Observer();
    };
    friend class Observer;

    KeyValueBroker();
    virtual ~KeyValueBroker();

    void publish(
	char const *	key,
	char const *	value,
	bool		set = true);

private:
    std::recursive_mutex mutex;
    typedef std::set<Observer const *> Observers;
    std::map<std::string, Observers *> observersFor;

    void subscribe(Observer const & observer);
    void unsubscribe(Observer const & observer);

protected:
    virtual void set(char const * key, char const * value) = 0;
    virtual bool get(char const * key, std::string & value) = 0;
};


#endif