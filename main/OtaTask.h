#ifndef OtaTask_h_
#define OtaTask_h_

#include "AsioTask.h"
#include "KeyValueBroker.h"
#include "Timer.h"

/// OtaTask is a StoppableTask
/// whose run method continually tries to perform
/// an Over The Air (OTA) firmware update using secure HTTP
class OtaTask : public AsioTask {
private:
    std::string				url;
    std::string				certificate;
    Timer				retryTimer;
    KeyValueBroker &			keyValueBroker;
    KeyValueBroker::Observer const	otaUrlObserver;
    KeyValueBroker::Observer const	otaRetryObserver;
    unsigned				otaStartObserverEntered;
    KeyValueBroker::Observer const	otaStartObserver;

public:
    OtaTask(
	char const *		url,
	char const *		certificate,
	unsigned		retry,
	KeyValueBroker &	keyValueBroker);

    void update();

    /* virtual */ void run() override;

    /* virtual */ ~OtaTask() override;
};

#endif
