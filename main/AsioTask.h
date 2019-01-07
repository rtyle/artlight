#pragma once

#include <asio.hpp>

#include "Task.h"

/// An AsioTask supports asynchronous I/O (using asio)
/// which it leverages for its StoppableTask implementation.
/// The asio::io_context can be used for general purpose communication
/// with the task by passing a lambda function to its post method.
/// This lambda function will be run as part of the task.
class AsioTask : public StoppableTask {
private:
    asio::io_context joinableIo;
    asio::io_context::work work;

protected:
    asio::io_context io;

public:
    AsioTask(
	char const *	name,
	UBaseType_t	priority,
	size_t		stackSize,
	BaseType_t	core = tskNO_AFFINITY);

    AsioTask();

    /* virtual */ void run() override;

    /* virtual */ void join() override;

    /* virtual */ void stop(bool join = true) override;

    virtual ~AsioTask();
};
