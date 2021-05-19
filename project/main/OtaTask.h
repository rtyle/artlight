#pragma once

#include "AsioTask.h"
#include "KeyValueBroker.h"

/// OtaTask is a StoppableTask
/// whose run method continually tries to perform
/// an Over The Air (OTA) firmware update using secure HTTP
class OtaTask : public AsioTask {
private:
    std::string				url;
    std::string				certificate;
    KeyValueBroker &			keyValueBroker;
    KeyValueBroker::Observer const	otaUrlObserver;
    unsigned				otaStartObserverEntered;
    KeyValueBroker::Observer const	otaStartObserver;

public:
    OtaTask(
	char const *		url,
	char const *		certificate,
	KeyValueBroker &	keyValueBroker);

    void update();

    /* virtual */ void run() override;

    /* virtual */ ~OtaTask() override;
};
