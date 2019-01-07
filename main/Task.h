#pragma once

#include <cstdlib>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/// A Task, once start'ed, creates a FreeRTOS task
/// in which its virtual run method is invoked.
class Task {
private:
    static void runThat(void * that);

protected:
    char const *		name;
    UBaseType_t const		priority;
    size_t const		stackSize;
    BaseType_t const		core;

    TaskHandle_t		taskHandle;

    /// Derivations should override run
    virtual void run();

public:
    Task(
	char const *	name,
	UBaseType_t	priority,
	size_t		stackSize,
	BaseType_t	core = tskNO_AFFINITY);

    Task();

    /// Start running this Task
    void start();

    virtual ~Task();
};

/// A StoppableTask is an abstraction for a derivation that can be stopped
/// and joined.
class StoppableTask : public Task {
public:
    StoppableTask(
	char const *	name,
	UBaseType_t	priority,
	size_t		stackSize,
	BaseType_t	core = tskNO_AFFINITY);

    StoppableTask();

    virtual void join() = 0;

    virtual void stop(bool join = true) = 0;
};
