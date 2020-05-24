#pragma once

#include <atomic>

#include "AsioTask.h"
#include "LuxSensor.h"
#include "Timer.h"

/// continually monitor the ambient lux level
/// and provide the latest measurement.
class LuxTask : public AsioTask {
private:
    LuxSensor &		luxSensor;
    Timer		timer;
    std::atomic<float>	lux;

    void update();

public:
    LuxTask(LuxSensor & luxSensor);

    /* virtual */ void run() override;

    float getLux();
};
