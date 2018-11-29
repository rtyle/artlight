#ifndef AsioTask_h_
#define AsioTask_h_

#include <asio.hpp>

#include "Task.h"

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
	StackType_t *	stack,
	size_t		stackSize);

    AsioTask();

    /* virtual */ void run() override;

    /* virtual */ void join() override;

    /* virtual */ void stop(bool join = true) override;

    virtual ~AsioTask();
};

#endif
