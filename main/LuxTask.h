#ifndef LuxTask_h_
#define LuxTask_h_

#include <atomic>

#include "AsioTask.h"
#include "I2C.h"
#include "TSL2561.h"
#include "Timer.h"

/// continually monitor the ambient lux level
/// and provide the latest measurement.
class LuxTask : public AsioTask {
private:
    float		luxBy[3];
    size_t		integrationTime;
    TSL2561		tsl2561;
    Timer		timer;
    std::atomic<float>	lux;

    void update();

public:
    LuxTask(I2C::Master const * i2cMaster);

    /* virtual */ void run() override;

    float getLux();
};

#endif
