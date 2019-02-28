#include <esp_err.h>

#include "Pin.h"

std::recursive_mutex Pin::classMutex;
std::vector<bool> Pin::pinUsed(GPIO_NUM_MAX, false);

Pin::Pin(
    gpio_num_t		gpio_num_,
    gpio_mode_t		mode,
    gpio_pullup_t	pull_up_en,
    gpio_pulldown_t	pull_down_en,
    gpio_int_type_t	intr_type)
:
    gpio_num	(gpio_num_)
{
    {
	std::lock_guard<std::recursive_mutex> lock(classMutex);
	if (pinUsed[gpio_num]) {
	    throw std::bad_alloc();
	}
	pinUsed[gpio_num] = true;
    }
    gpio_config_t config = {
	static_cast<uint64_t>(1) << gpio_num,
	mode,
	pull_up_en,
	pull_down_en,
	intr_type,
    };
    ESP_ERROR_CHECK(gpio_config(&config));
}

Pin::~Pin() {
    std::lock_guard<std::recursive_mutex> lock(classMutex);
    pinUsed[gpio_num] = false;
}

void Pin::reset() {
    ESP_ERROR_CHECK(gpio_reset_pin(gpio_num));
}

void Pin::set_intr_type(gpio_int_type_t intr_type) {
    ESP_ERROR_CHECK(gpio_set_intr_type(gpio_num, intr_type));
}

void Pin::intr_enable() {
    ESP_ERROR_CHECK(gpio_intr_enable(gpio_num));
}

void Pin::intr_disable() {
    ESP_ERROR_CHECK(gpio_intr_disable(gpio_num));
}

void Pin::set_level(uint32_t level) {
    ESP_ERROR_CHECK(gpio_set_level(gpio_num, level));
}

int Pin::get_level() {
    return gpio_get_level(gpio_num);
}

void Pin::set_direction(gpio_mode_t mode) {
    ESP_ERROR_CHECK(gpio_set_direction(gpio_num, mode));
}

void Pin::set_pull_mode(gpio_pull_mode_t pull) {
    ESP_ERROR_CHECK(gpio_set_pull_mode(gpio_num, pull));
}

void Pin::wakeup_enable(gpio_int_type_t intr_type) {
    ESP_ERROR_CHECK(gpio_wakeup_enable(gpio_num, intr_type));
}

void Pin::wakeup_disable() {
    ESP_ERROR_CHECK(gpio_wakeup_disable(gpio_num));
}

void Pin::pullup_en() {
    ESP_ERROR_CHECK(gpio_pullup_en(gpio_num));
}

void Pin::pullup_dis() {
    ESP_ERROR_CHECK(gpio_pullup_dis(gpio_num));
}

void Pin::pulldown_en() {
    ESP_ERROR_CHECK(gpio_pulldown_en(gpio_num));
}

void Pin::pulldown_dis() {
    ESP_ERROR_CHECK(gpio_pulldown_dis(gpio_num));
}

void Pin::set_drive_capability(gpio_drive_cap_t strength) {
    ESP_ERROR_CHECK(gpio_set_drive_capability(gpio_num, strength));
}

void Pin::hold_en() {
    ESP_ERROR_CHECK(gpio_hold_en(gpio_num));
}

void Pin::hold_dis() {
    ESP_ERROR_CHECK(gpio_hold_dis(gpio_num));
}

void Pin::iomux_in(uint32_t signal_idx) {
    gpio_iomux_in(gpio_num, signal_idx);
}

void Pin::iomux_out(int func, bool oen_inv) {
    gpio_iomux_out(gpio_num, func, oen_inv);
}

ObservablePin::ISR::ISR(int intr_alloc_flags) {
    gpio_install_isr_service(intr_alloc_flags);
}

ObservablePin::ISR::~ISR() {
    gpio_uninstall_isr_service();
}

ObservablePin::Task::Task(
    char const *	name,
    UBaseType_t		priority,
    size_t		stackSize,
    BaseType_t		core,
    size_t		size)
:
    Qio::Task(name, priority, stackSize, core, size)
{}

ObservablePin::Task::~Task() {}

ObservablePin::Observer::Observer(
    ObservablePin & observablePin_,
    std::function<void()> observe_)
:
    observablePin	(observablePin_),
    observe		(observe_)
{
    std::lock_guard<std::recursive_mutex> lock(observablePin.mutex);
    observablePin.observers.insert(this);
    if (1 == observablePin.observers.size()) {
	// this is the first observer on its observablePin
	// A. call ObservablePin::postFromISRThat(&observablePin) on interrupt
	gpio_isr_handler_add(observablePin.gpio_num,
	    postFromISRFor, &observablePin);
    }
}

void ObservablePin::postFromISRFor(void * that) {
    // B. forward from class method to instance method for that
    static_cast<ObservablePin *>(that)->postFromISR();
}

void ObservablePin::postFromISR() {
    // C. observeFromISR, as constructed ...
    observeFromISR(this);
}

ObservablePin::Task::operator std::function<void(ObservablePin *)>() {
    return [this](ObservablePin * observablePin) {
	// ... if ObservablePin was constructed with a copy of this:
	// D. dispatch observablePin->observeFromTask object from our task
	io.postFromISR(&observablePin->observeFromTask);
    };
}

ObservablePin::ObservablePin(
    gpio_num_t					gpio_num,
    gpio_mode_t					mode,
    gpio_pullup_t				pull_up_en,
    gpio_pulldown_t				pull_down_en,
    gpio_int_type_t				intr_type,
    std::function<void(ObservablePin *)>	observeFromISR_)
:
    Pin(gpio_num, mode, pull_up_en, pull_down_en, intr_type),
    observeFromISR(observeFromISR_),
    observeFromTask([this](){
	// E. for each observer
	observe();
    })
{}

ObservablePin::ObservablePin(ObservablePin const && move)
:
    Pin(move),
    observeFromISR(move.observeFromISR),
    observeFromTask(move.observeFromTask)
{}

void ObservablePin::observe() {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    for (auto & observer: observers) {
	// F. for this observer
	(*observer)();
    }
}

void ObservablePin::Observer::operator()() {
    // G. observe, as constructed ...
    observe();
}

ObservablePin::Observer::~Observer() {
    std::lock_guard<std::recursive_mutex> lock(observablePin.mutex);
    observablePin.observers.erase(this);
    if (0 == observablePin.observers.size()) {
	// this was the last Observer on its observablePin
	gpio_isr_handler_remove(observablePin.gpio_num);
    }
}
