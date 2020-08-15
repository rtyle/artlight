#include "SensorTask.h"

SensorTask::SensorTask()
:
    AsioTask {"sensorTask", 5, 4096, 0}
{}

/* virtual */ void SensorTask::run() {
    // create some dummy work ...
    asio::io_service::work work(io);

    // ... so that we will run forever
    AsioTask::run();
}
