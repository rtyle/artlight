#pragma once

#include <functional>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "Task.h"

/// The Qio namespace supports
/// a simple Qio::IO class (modeled after asio::context_io) and
/// a simple Qio::Task (modeled after AsioTask).
/// Qio::IO only supports input from a FreeRTOS queue.
/// Qio::IO::run reads pointers to nullary functions and invokes them.
/// These can be queued by Qio::IO::post or Qio::IO::postFromISR.
namespace Qio {

class IO {
private:
    QueueHandle_t const		queue;
    std::function<void()> const	stopAction;
    bool			stopped;
public:
    IO(size_t size);
    ~IO();
    void post(std::function<void()> const * action);
    void postFromISR(std::function<void()> const * action);
    void run();
    void stop();
};

class Task: public StoppableTask {
private:
    SemaphoreHandle_t	joinable;
protected:
    IO	io;
    void run() override;
public:
    Task(
	char const *	name,
	UBaseType_t	priority,
	size_t		stackSize,
	BaseType_t	core,
	size_t		size);
    void join() override;
    void stop(bool join = true) override;
    virtual ~Task();
};

}
