#ifndef LuxMonitorTask_h_
#define LuxMonitorTask_h_

#include <atomic>

#include "AsioTask.h"
#include "I2C.h"
#include "TSL2561.h"

/// continually monitor the ambient lux level
/// and provide the latest measurement.
class LuxMonitorTask : public AsioTask {
private:
    TSL2561		tsl2561;
    std::atomic<float>	lux;

    void update();

public:
    LuxMonitorTask(I2C::Master const * i2cMaster);

    /* virtual */ void run() override;

    float getLux();
};

#endif
