#ifndef Task_h_
#define Task_h_

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
    StackType_t * const		stack;
    size_t const		stackSize;

    StaticTask_t		taskStatic;
    TaskHandle_t		taskHandle;

    /// Derivations should override run
    virtual void run();

public:
    Task(
	char const *	name,
	UBaseType_t	priority,
	StackType_t *	stack,
	size_t		stackSize);

    Task();

    /// Start running this Task
    void start();

    virtual ~Task();
};

class StoppableTask : public Task {
public:
    StoppableTask(
	char const *	name,
	UBaseType_t	priority,
	StackType_t *	stack,
	size_t		stackSize);

    StoppableTask();

    virtual void join() = 0;

    virtual void stop(bool join = true) = 0;
};

#endif
