#pragma once

#include <functional>
#include <map>
#include <mutex>
#include <set>

#include <esp_event_legacy.h>
#include <esp_event_loop.h>

// the default event loop, created by
//	esp_event_loop_init
// starts a FreeRTOS task ("eventTask") with an implementation that runs forever.
// this task reacts to
//	system_event_t
// objects posted to its queue by
//	esp_event_send
// its reaction is to first call
//	esp_event_process_default
// with the event and then call
//	esp_event_post_to_user
// the return values from these do not affect control;
// they are only used to ESP_LOGE an error message if they are not ESP_OK.
//	esp_event_process_default
// indexes a
// 	static system_event_handler_t default_event_handlers[SYSTEM_EVENT_MAX]
// the
//	typedef enum {..., SYSTEM_EVENT_MAX} system_event_id_t
// enumeration is defined by the system.
//	default_event_handlers
// is populated by
//	esp_event_set_default_wifi_handlers
//	esp_event_set_default_eth_handlers
// which are presumably called by related components.
//	esp_event_post_to_user
// calls a
//	system_event_cb_t s_event_handler_cb
// that was remembered when
//	esp_event_loop_init
// was called or overridden later by
//	esp_event_loop_set_cb
//
// WARNING:
// this default event loop implementation seems to be in a transition
// to a specialization of a more general event loop capability.

namespace Event {

    class Observer {
    public:
	using Observe = std::function<esp_err_t(system_event_t const *)>;
	system_event_id_t const	id;
	Observe const		observe;
	Observer(system_event_id_t, Observe && observe);
	esp_err_t operator()(system_event_t const *) const;
	~Observer();
    };

    /// Reactor is a singleton class whose only instance
    /// will reactTo each event dispatched by the system's default event loop.
    /// Each event's system_event_id_t is, potentially, mapped to
    /// an ObserverMap for which the Action of each is taken.
    class Reactor {
    private:
	friend class Observer;

	esp_err_t reactTo(system_event_t const *);
	static esp_err_t reactToThat(void * that, system_event_t *);

	std::mutex mutex;
	using Observers = std::set<Observer const *>;
	std::map<system_event_id_t, Observers *> observersFor;

	Reactor();
	static Reactor * reactor;
	static Reactor * getReactor();
	virtual ~Reactor();

	void subscribe(Observer const & observer);
	void unsubscribe(Observer const & observer);
    };

}
