#include <esp_log.h>

#include "AsioTask.h"

/* virtual */ void AsioTask::run() {
    ESP_LOGI(name, "AsioTask::run");
    io.run();
    joinableIo.post([this](){
	ESP_LOGI(name, "AsioTask::run stopped");
    });
}

AsioTask::AsioTask(
    char const *	name,
    UBaseType_t		priority,
    StackType_t *	stack,
    size_t		stackSize)
:
    StoppableTask(name, priority, stack, stackSize),
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
    joinableIo.run_one();
    taskHandle = nullptr;
}

/* virtual */ void AsioTask::stop(bool join_) {
    if (taskHandle) {
	ESP_LOGI(name, "AsioTask::stop");
	io.stop();
	if (join_) join();
    }
}

/* virtual */ AsioTask::~AsioTask() {
    stop();
    ESP_LOGI(name, "AsioTask::~AsioTask");
}
