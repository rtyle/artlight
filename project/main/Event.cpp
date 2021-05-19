#include "esp_log.h"

#include "Error.h"
#include "Event.h"

namespace Event {

    struct LoopArgs: public esp_event_loop_args_t {
	LoopArgs();
	#define setter(name) LoopArgs & name##_(decltype(name) s);
	setter(queue_size)
	setter(task_name)
	setter(task_priority)
	setter(task_stack_size)
	setter(task_core_id)
	#undef setter
    };

    Loop::Loop(): id{nullptr} {
	Error::throwIf(esp_event_loop_create_default());
    }

    Loop::Loop(
	int32_t		queue_size,
	char const *	task_name,
	UBaseType_t	task_priority,
	uint32_t	task_stack_size,
	BaseType_t	task_core_id)
    :
	id{[queue_size, task_name, task_priority, task_stack_size, task_core_id](){
	    esp_event_loop_handle_t id;
	    Error::throwIf(esp_event_loop_create(
		&LoopArgs()
		    .queue_size_	(queue_size)
		    .task_name_		(task_name)
		    .task_priority_	(task_priority)
		    .task_stack_size_	(task_stack_size)
		    .task_core_id_	(task_core_id),
		&id)
	    );
	    return id;
	}()}
    {}

    Loop::~Loop() {
	if (id) {
	    Error::throwIf(esp_event_loop_delete(id));
	} else {
	    Error::throwIf(esp_event_loop_delete_default());
	}
    }

    Loop::operator esp_event_loop_handle_t() const {return id;}

    Handler::Handler(
	esp_event_loop_handle_t	loop_,
	esp_event_base_t	base_,
	int32_t			id_,
	Handle &&		handle_)
    :
	loop		{loop_},
	base		{base_},
	id		{id_},
	handle		{std::move(handle_)},
	registration	{[this](){
	    esp_event_handler_instance_t registration {nullptr};
	    if (loop) {
		Error::throwIf(esp_event_handler_instance_register_with(
		    loop,
		    base,
		    id,
		    handleThat,
		    this,
		    &registration)
		);
	    } else {
		Error::throwIf(esp_event_handler_instance_register(
		    base,
		    id,
		    handleThat,
		    this,
		    &registration)
		);
	    }
	    return registration;
	}()}
    {}

    void Handler::handleThat(
	void *			that,
	esp_event_base_t	base,
	int32_t			id,
	void *			event)
    {
	static_cast<Handler *>(that)->handle(base, id, event);
    }

    Handler::~Handler() {
	if (registration) {
	    if (loop) {
		Error::throwIf(esp_event_handler_instance_unregister_with(
		    loop,
		    base,
		    id,
		    registration)
		);
	    } else {
		Error::throwIf(esp_event_handler_instance_unregister(
		    base,
		    id,
		    registration)
		);
	    }
	}
    }

}
