#include "esp_log.h"

#include "Error.h"
#include "Task.h"

// We would have liked to created a "static" task:
// one whose stack and Task Control Block (TCB) resources
// are owned by/in a Task instance.
// The trouble with this is synchronizing the destruction of this instance's
// resources (its destructor) with the system's use of them.
//
// Unfortunately, calling FreeRTOS vTaskDelete will not cause immediate
// release of the system's use of the task's TCB.
// Rather, it will delay this until *an* idle task
// (*the* idle task running on the same core if the task is pinned)
// gets around to releasing it.
// There does not seem to be a good/clean way to determine when that is
// and wait for it; even if we would want to wait for it.
//
// The *esp-idf port* of FreeRTOS vTaskDelete (freertos/tasks.c)
//	... will immediately free task memory if the task being deleted is
//	NOT currently running and not pinned to the other core.
//	Otherwise, freeing of task memory
//	will still be delegated to the Idle Task.
// We could get this to work if
//	1. A task vTaskSuspend'ed itself on exit (becomes no longer running)
//	2. vTaskDelete is called from the Task destructor in a different task
//	3. The task does not use the FPU
//		and was constructed as not pinned to a any core OR
//	4. The task was constructed as pinned to the core the destructor runs in
// Using the FPU (3) will cause an otherwise unpinned task to become pinned
// the core it is running on.
// 3 and 4 are unreasonable restrictions.
//
// So ...  we do not create a "static" task.

/* static */ void Task::runThat(void * that) {
    Task * thatTask = static_cast<Task *>(that);
    ESP_LOGI(thatTask->name, "%d %s Task::runThat",
	xPortGetCoreID(), pcTaskGetTaskName(nullptr));
    thatTask->run();
    ESP_LOGI(thatTask->name, "%d %s Task::runThat task delete",
	xPortGetCoreID(), pcTaskGetTaskName(nullptr));
    vTaskDelete(nullptr);
}

/* virtual */ void Task::run() {
    ESP_LOGI(name, "%d %s Task::run",
	xPortGetCoreID(), pcTaskGetTaskName(nullptr));
}

Task::Task(
    char const *	name_,
    UBaseType_t		priority_,
    size_t		stackSize_,
    BaseType_t		core_)
:
    name(name_),
    priority(priority_),
    stackSize(stackSize_),
    core(core_),
    taskHandle(nullptr)
{
    ESP_LOGI(name, "%d %s Task::Task",
	xPortGetCoreID(), pcTaskGetTaskName(nullptr));
}

Task::Task()
:
    name(pcTaskGetTaskName(nullptr)),
    priority(uxTaskPriorityGet(nullptr)),
    stackSize(0),
    core(0),
    taskHandle(xTaskGetCurrentTaskHandle())
{
}

void Task::start() {
    ESP_LOGI(name, "%d %s Task::start",
	xPortGetCoreID(), pcTaskGetTaskName(nullptr));
    Error::throwIf(pdPASS != xTaskCreatePinnedToCore(
	runThat, name, stackSize, this, priority, &taskHandle, core));
}

/* virtual */ Task::~Task() {
    ESP_LOGI(name, "%d %s Task::~Task",
	xPortGetCoreID(), pcTaskGetTaskName(nullptr));
}

StoppableTask::StoppableTask(
    char const *	name,
    UBaseType_t		priority,
    size_t		stackSize,
    BaseType_t		core)
:
    Task(name, priority, stackSize, core)
{}

StoppableTask::StoppableTask() : Task() {}
