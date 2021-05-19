#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "Qio.h"

namespace Qio {

IO::IO(size_t size)
:
    queue(xQueueCreate(size, sizeof(void *))),
    stopAction([this](void){this->stopped = true;}),
    stopped(false)
{}

IO::~IO() {
    vQueueDelete(queue);
}

void IO::post(std::function<void()> const * action) {
    xQueueSend(queue, &action, portMAX_DELAY);
}

void IO::postFromISR(std::function<void()> const * action) {
    BaseType_t yield;
    xQueueSendFromISR(queue, &action, &yield);
    if (yield) portYIELD_FROM_ISR();
}

void IO::run() {
    while (!stopped) {
	std::function<void()> const * action;
	if (xQueueReceive(queue, &action, portMAX_DELAY)) {
	    (*action)();
	}
    }
}

void IO::stop() {
    post(&stopAction);
}

Task::Task(
    char const *	name,
    UBaseType_t		priority,
    size_t		stackSize,
    BaseType_t		core,
    size_t		size)
:
    StoppableTask(name, priority, stackSize, core),
    joinable(xSemaphoreCreateBinary()),
    io(size)
{}

void Task::run() {
    ESP_LOGI(name, "%d %s Qsio::Task::run",
	xPortGetCoreID(), pcTaskGetTaskName(nullptr));
    io.run();
    ESP_LOGI(name, "%d %s Qsio::Task::run joinable",
	xPortGetCoreID(), pcTaskGetTaskName(nullptr));
    xSemaphoreGive(joinable);
}

void Task::join() {
    ESP_LOGI(name, "%d %s Qsio::Task::join",
	xPortGetCoreID(), pcTaskGetTaskName(nullptr));
    xSemaphoreTake(joinable, portMAX_DELAY);
    ESP_LOGI(name, "%d %s Qsio::Task::join joined",
	xPortGetCoreID(), pcTaskGetTaskName(nullptr));
    taskHandle = nullptr;
}

void Task::stop(bool join_) {
    if (taskHandle) {
	ESP_LOGI(name, "%d %s Qsio::Task::stop",
	    xPortGetCoreID(), pcTaskGetTaskName(nullptr));
	io.stop();
	if (join_) join();
    }
}

Task::~Task() {
    stop();
    vSemaphoreDelete(joinable);
}

}
