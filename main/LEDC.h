#pragma once

#include <stdexcept>
#include <forward_list>

#include "driver/ledc.h"

/// The LEDC namespace provides wrappers for ESP-IDF LEDC functions.
namespace LEDC {

/// ISR installs the fade function as the LEDC ISR which runs
/// on the CPU core that the it is constructed on.
/// The fade function is uninstalled on destruction.
class ISR {
public:
    ISR(int intr_alloc_flags = 0);
    ~ISR();
};

class Channel;

/// Timer wraps LEDC timer functions.
/// Construction dynamically allocates a free timer, destruction releases it.
/// std::bad_alloc is thrown when there are none free to allocate.
class Timer {
    friend Channel;
private:
    static std::forward_list<ledc_timer_t> free[LEDC_SPEED_MODE_MAX];
    ledc_mode_t const		speed_mode;
    ledc_timer_t const		timer_num;
public:
    Timer(
	ledc_mode_t		speed_mode	= LEDC_HIGH_SPEED_MODE,
	ledc_timer_bit_t	duty_resolution	= LEDC_TIMER_8_BIT,
	uint32_t		freq_hz	= 0 /* max for duty_resolution */
	#ifdef ESP_IDF_VERSION
	#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
	    ,
	    ledc_clk_cfg_t		clk_cfg	= LEDC_AUTO_CLK
	#endif
	#endif
    );
    ~Timer();
    void set_freq(uint32_t freq_hz);
    uint32_t get_freq();
    void rst();
    void pause();
    void resume();
};

/// Channel wraps LEDC channel functions.
/// Construction dynamically allocates a free channel, destruction releases it.
/// std::bad_alloc is thrown when there are none free to allocate.
class Channel {
private:
    static std::forward_list<ledc_channel_t> free[LEDC_SPEED_MODE_MAX];
    ledc_mode_t const		speed_mode;
    ledc_channel_t const	channel;
public:
    Channel(
	Timer const &		timer,
	int			gpio_num,
	uint32_t		duty,
	int			hpoint	= 0,
	ledc_intr_type_t	intr_type = LEDC_INTR_DISABLE);
    ~Channel();
    void bind_timer(Timer const & timer);
    void stop(uint32_t idle_level);

    // change PWM duty by software
    void set_duty(uint32_t duty);
    void set_duty_with_hpoint(uint32_t duty, uint32_t hpoint);
    void update_duty();
    int get_hpoint();
    uint32_t get_duty();

    // change PWM duty with hardware fading
    // (requires ISR installed by Fader construction).
    // higher speed timers cannot support longer  fades and
    // lower  speed timers cannot support shorter fades.
    void set_fade_with_time(uint32_t target_duty, int max_fade_time_ms);
    void set_fade_with_step(uint32_t target_duty, uint32_t scale,
	uint32_t cycle_num);
    void set_fade(uint32_t duty, ledc_duty_direction_t fade_direction,
	uint32_t step_num, uint32_t duty_cycle_num, uint32_t duty_scale);
    void fade_start(ledc_fade_mode_t fade_mode = LEDC_FADE_NO_WAIT);

    // thread safe versions
    void set_duty_and_update(uint32_t duty, uint32_t hpoint);
    void set_fade_time_and_start(uint32_t target_duty,
	uint32_t max_fade_time_ms, ledc_fade_mode_t fade_mode);
    void set_fade_step_and_start(uint32_t target_duty, uint32_t scale,
	uint32_t cycle_num, ledc_fade_mode_t fade_mode);
};

}
