#pragma once

#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <vector>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include <driver/gpio.h>

#include "Qio.h"

/// Pin thinly wraps the notion of a GPIO pin
class Pin {
private:
    static std::recursive_mutex classMutex;
    static std::vector<bool> pinUsed;
public:
    gpio_num_t const gpio_num;
    Pin(
	gpio_num_t	gpio_num_,
	gpio_mode_t	mode,
	gpio_pullup_t	pull_up_en,
	gpio_pulldown_t	pull_down_en,
	gpio_int_type_t	intr_type	= GPIO_INTR_DISABLE);
    ~Pin();

    void reset();
    void set_intr_type(gpio_int_type_t intr_type);
    void intr_enable();
    void intr_disable();
    void set_level(uint32_t level);
    int get_level();
    void set_direction(gpio_mode_t mode);
    void set_pull_mode(gpio_pull_mode_t pull);
    void wakeup_enable(gpio_int_type_t intr_type);
    void wakeup_disable();
    void pullup_en();
    void pullup_dis();
    void pulldown_en();
    void pulldown_dis();
    void set_drive_capability(gpio_drive_cap_t strength);
    void hold_en();
    void hold_dis();
    void iomux_in(uint32_t signal_idx);
    void iomux_out(int func, bool oen_inv);
};

/// An ObservablePin is a Pin that can have many ObservablePin::Observer's
/// These observers will be notified
class ObservablePin: public Pin {
public:

    /// ISR installs the GPIO ISR service which runs
    /// on the CPU core that the it is constructed on.
    /// The service is uninstalled on destruction.
    class ISR {
    public:
        ISR(int intr_alloc_flags = 0);
        ~ISR();
    };

    /// Create a ObservablePin::Observer to observe changes on an ObservablePin.
    /// For this to work, the GPIO ISR service needs to be installed
    /// (see ObservablePin::ISR) and the ObservablePin's observeFromISR needs
    /// to arrange for ObservablePin.observe to be called
    /// (one can use an ObservablePin::Task for this purpose).
    class Observer {
    private:
	ObservablePin & observablePin;
	std::function<void()> const observe;
    public:
	Observer(ObservablePin & observablePin, std::function<void()> observe);
	~Observer();
	void operator()();
    };
    friend Observer;

    /// An ObservablePin::Task can implicitly be converted to
    /// an observeFromISR function suitable for ObservablePin construction.
    /// This ISR function will queue a pointer to the ObservablePin's
    /// observeFromTask object which will be read and dispatched from the task.
    class Task : public Qio::Task {
    public:
	Task(
	    char const *	name,
	    UBaseType_t		priority,
	    size_t		stackSize,
	    BaseType_t		core,
	    size_t		size);
	~Task();
	operator std::function<void(ObservablePin *)>();
    };
    friend Task;

    ObservablePin(
	gpio_num_t				gpio_num_,
	gpio_mode_t				mode,
	gpio_pullup_t				pull_up_en,
	gpio_pulldown_t				pull_down_en,
	gpio_int_type_t				intr_type,
	std::function<void(ObservablePin *)>	observeFromISR);

    /// Notify each Observer
    void observe();

private:
    std::function<void(ObservablePin *)>	observeFromISR;
    std::function<void()> const			observeFromTask;
    std::recursive_mutex			mutex;
    std::set<Observer *>			observers;

    void postFromISR();
    static void postFromISRFor(void * that);
};
