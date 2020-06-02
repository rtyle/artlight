#pragma once

#include "AsioTask.h"

/// Task for synchronizing Sensor access
class SensorTask : public AsioTask {
public:
    SensorTask();

    operator asio::io_context & ();

    /* virtual */ void run() override;
};
