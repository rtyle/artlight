#ifndef OtaTask_h_
#define OtaTask_h_

#include "AsioTask.h"

/// OtaTask is a StoppableTask
/// whose run method continually tries to perform
/// an Over The Air (OTA) firmware update using secure HTTP
class OtaTask : public AsioTask {
private:
    char const * const	url;
    char const * const	cert;
    unsigned const	retry;

public:
    OtaTask(
	char const *	url,
	char const *	cert,
	unsigned	retry);

    void update();

    /* virtual */ void run() override;

    /* virtual */ ~OtaTask() override;
};

#endif
