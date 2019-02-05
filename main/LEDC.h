#pragma once

#include <stdexcept>
#include <forward_list>

#include <driver/ledc.h>

/// The LEDC namespace provides wrappers for ESP-IDF LEDC functions.
/// Timer functions are wrapped through the Timer class.
/// Channel functions are wrapped through the Channel class.
/// Timer/Channel construction dynamically allocate the next free
/// timer_num/channel for the speed_mode requested.
/// std::bad_alloc is thrown when there are none free.
/// Timer/Channel destruction returns the timer_num/channel to the free pool.
namespace LEDC {

class Channel;

class Timer {
    friend Channel;
private:
    static std::forward_list<ledc_timer_t> free[LEDC_SPEED_MODE_MAX];
    ledc_mode_t const		speed_mode;
    ledc_timer_t const		timer_num;
public:
    Timer(
	ledc_mode_t		speed_mode,
	ledc_timer_bit_t	duty_resolution	= LEDC_TIMER_8_BIT,
	uint32_t		freq_hz	= 0 /* max for duty_resolution */);
    ~Timer();
    void set_freq(uint32_t freq_hz);
    uint32_t get_freq();
    void rst();
    void pause();
    void resume();
};

class Channel {
private:
    static std::forward_list<ledc_channel_t> free[LEDC_SPEED_MODE_MAX];
    ledc_mode_t const		speed_mode;
    ledc_channel_t const	channel;
public:
    Channel(
	Timer const &		timer,
	int			gpio_num,
	ledc_intr_type_t	intr_type,
	uint32_t		duty,
	int			hpoint	= 0);
    ~Channel();
    void bind_timer(Timer const & timer);
    void stop(uint32_t idle_level);

    // not thread safe:
    void set_duty(uint32_t duty);
    void set_duty_with_hpoint(uint32_t duty, uint32_t hpoint);
    void update_duty();

    int get_hpoint();
    uint32_t get_duty();
    void set_fade(uint32_t duty, ledc_duty_direction_t fade_direction,
	uint32_t step_num, uint32_t duty_cycle_num, uint32_t duty_scale);

    // not thread safe:
    void set_fade_with_step(uint32_t target_duty, uint32_t scale,
	uint32_t cycle_num);
    void set_fade_with_time(uint32_t target_duty, int max_fade_time_ms);
    void fade_start(ledc_fade_mode_t fade_mode);

    // thread safe:
    void set_duty_and_update(uint32_t duty, uint32_t hpoint);
    void set_fade_time_and_start(uint32_t target_duty,
	uint32_t max_fade_time_ms, ledc_fade_mode_t fade_mode);
    void set_fade_step_and_start(uint32_t target_duty, uint32_t scale,
	uint32_t cycle_num, ledc_fade_mode_t fade_mode);
};

class Fade {
public:
    Fade();
    ~Fade();
};

}