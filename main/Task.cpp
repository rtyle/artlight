#include <esp_log.h>

#include "Task.h"

/* static */ void Task::runThat(void * that) {
    Task * thatTask = static_cast<Task *>(that);
    ESP_LOGI(thatTask->name, "%d %s Task::runThat",
	xPortGetCoreID(), pcTaskGetTaskName(nullptr));
    thatTask->run();
    ESP_LOGI(thatTask->name, "%d %s Task::runThat stopped",
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
    StackType_t *	stack_,
    size_t		stackSize_,
    BaseType_t		core_)
:
    name(name_),
    priority(priority_),
    stack(stack_),
    stackSize(stackSize_),
    core(core_),
    taskStatic({}),
    taskHandle(nullptr)
{
    ESP_LOGI(name, "%d %s Task::Task",
	xPortGetCoreID(), pcTaskGetTaskName(nullptr));
}

Task::Task()
:
    name(pcTaskGetTaskName(nullptr)),
    priority(uxTaskPriorityGet(nullptr)),
    stack(nullptr),
    stackSize(0),
    core(0),
    taskStatic({}),
    taskHandle(xTaskGetCurrentTaskHandle())
{
}

void Task::start() {
    ESP_LOGI(name, "%d %s Task::start",
	xPortGetCoreID(), pcTaskGetTaskName(nullptr));
    taskHandle = xTaskCreateStaticPinnedToCore(
	runThat, name, stackSize, this, priority, stack, &taskStatic, core);
}

/* virtual */ Task::~Task() {
    ESP_LOGI(name, "%d %s Task::~Task",
	xPortGetCoreID(), pcTaskGetTaskName(nullptr));
}

StoppableTask::StoppableTask(
    char const *	name,
    UBaseType_t		priority,
    StackType_t *	stack,
    size_t		stackSize,
    BaseType_t		core)
:
    Task(name, priority, stack, stackSize, core)
{}

StoppableTask::StoppableTask() : Task() {}
