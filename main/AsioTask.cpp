#include <esp_log.h>

#include "AsioTask.h"

/* virtual */ void AsioTask::run() {
    ESP_LOGI(name, "%d %s AsioTask::run",
	xPortGetCoreID(), pcTaskGetTaskName(nullptr));
    io.run();
    ESP_LOGI(name, "%d %s AsioTask::run joinable",
	xPortGetCoreID(), pcTaskGetTaskName(nullptr));
    joinableIo.post([this](){
	ESP_LOGI(name, "%d %s AsioTask::run stopped",
	    xPortGetCoreID(), pcTaskGetTaskName(nullptr));
    });
}

AsioTask::AsioTask(
    char const *	name,
    UBaseType_t		priority,
    StackType_t *	stack,
    size_t		stackSize,
    BaseType_t		core)
:
    StoppableTask(name, priority, stack, stackSize, core),
    joinableIo(),
    work(joinableIo),
    io()
{}

AsioTask::AsioTask()
:
    StoppableTask(),
    joinableIo(),
    work(joinableIo),
    io()
{}

/* virtual */ void AsioTask::join() {
    ESP_LOGI(name, "%d %s AsioTask::join",
	xPortGetCoreID(), pcTaskGetTaskName(nullptr));
    joinableIo.run_one();
    ESP_LOGI(name, "%d %s AsioTask::join joined",
	xPortGetCoreID(), pcTaskGetTaskName(nullptr));
    taskHandle = nullptr;
}

/* virtual */ void AsioTask::stop(bool join_) {
    if (taskHandle) {
	ESP_LOGI(name, "%d %s AsioTask::stop",
	    xPortGetCoreID(), pcTaskGetTaskName(nullptr));
	io.stop();
	if (join_) join();
    }
}

/* virtual */ AsioTask::~AsioTask() {
    stop();
    ESP_LOGI(name, "%d %s AsioTask::~AsioTask",
	xPortGetCoreID(), pcTaskGetTaskName(nullptr));
}
