#include <esp_log.h>

#include "Task.h"

/* static */ void Task::runThat(void * that) {
    Task * thatTask = static_cast<Task *>(that);
    ESP_LOGI(thatTask->name, "Task::runThat");
    thatTask->run();
    ESP_LOGI(thatTask->name, "Task::runThat stopped");
    vTaskDelete(nullptr);
}

/* virtual */ void Task::run() {
    ESP_LOGI(name, "Task::run");
}

Task::Task(
    char const *	name_,
    UBaseType_t		priority_,
    StackType_t *	stack_,
    size_t		stackSize_)
:
    name(name_),
    priority(priority_),
    stack(stack_),
    stackSize(stackSize_),
    taskStatic({}),
    taskHandle(nullptr)
{
    ESP_LOGI(name, "Task::Task");
}

Task::Task()
:
    name(pcTaskGetTaskName(nullptr)),
    priority(uxTaskPriorityGet(nullptr)),
    stack(nullptr),
    stackSize(0),
    taskStatic({}),
    taskHandle(xTaskGetCurrentTaskHandle())
{
}

void Task::start() {
    ESP_LOGI(name, "Task::start");
    taskHandle = xTaskCreateStatic(
	runThat, name, stackSize, this, priority, stack, &taskStatic);
}

/* virtual */ Task::~Task() {
    ESP_LOGI(name, "Task::~Task");
}

StoppableTask::StoppableTask(
    char const *	name,
    UBaseType_t		priority,
    StackType_t *	stack,
    size_t		stackSize)
:
    Task(name, priority, stack, stackSize)
{}

StoppableTask::StoppableTask() : Task() {}
