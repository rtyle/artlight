#pragma once

#include "AsioTask.h"

/// Task for synchronizing Sensor access
class SensorTask : public AsioTask {
public:
    SensorTask();

    /* virtual */ void run() override;
};
