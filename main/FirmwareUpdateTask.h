#ifndef FirmwareUpdateTask_h_
#define FirmwareUpdateTask_h_

#include "AsioTask.h"

/// FirmwareUpdateTask is a StoppableTask
/// whose run method continually tries to perform
/// an Over The Air (OTA) firmware update using secure HTTP
class FirmwareUpdateTask : public AsioTask {
private:
    StackType_t		stack[4096] = {};
    char const * const	url;
    char const * const	cert;
    TickType_t const	pause;

public:
    FirmwareUpdateTask(
	char const *	url,
	char const *	cert,
	TickType_t	pause);

    void update();

    /* virtual */ void run() override;

    /* virtual */ ~FirmwareUpdateTask() override;
};

#endif
