#pragma once

#include <cstdint>
#include <functional>

#include "esp_event.h"

namespace Event {

    class Loop {
    private:
	    esp_event_loop_handle_t const id;
    public:
	    Loop();
	    Loop(
		int32_t		queue_size,
		char const *	task_name,
		UBaseType_t	task_priority,
		uint32_t	task_stack_size,
		BaseType_t	task_core_id);
	    operator esp_event_loop_handle_t() const;
	    ~Loop();
    };

    class Handler {
    public:
	using Handle = std::function<void(esp_event_base_t base, int32_t id, void *)>;
    private:
	static void handleThat(
	    void *		that,
	    esp_event_base_t	base,
	    int32_t		id,
	    void *		event);
	esp_event_loop_handle_t const		loop;
	esp_event_base_t const			base;
	int32_t const				id;
	Handle const				handle;
	esp_event_handler_instance_t const	registration;
    public:
	Handler(
	    esp_event_loop_handle_t	loop,
	    esp_event_base_t		base,
	    int32_t			id,
	    Handle &&			handle);
	~Handler();
    };

}
