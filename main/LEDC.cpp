#include "LEDC.h"

namespace LEDC {

std::forward_list<ledc_timer_t> Timer::free[] {
    {
	LEDC_TIMER_0,
	LEDC_TIMER_1,
	LEDC_TIMER_2,
	LEDC_TIMER_3,
    },
    {
	LEDC_TIMER_0,
	LEDC_TIMER_1,
	LEDC_TIMER_2,
	LEDC_TIMER_3,
    },
};

Timer::Timer(
    ledc_mode_t		speed_mode,
    ledc_timer_bit_t	duty_resolution,
    uint32_t		freq_hz)
:
    config {
	speed_mode,
	duty_resolution,
	[speed_mode](){
	    auto free_ = free[speed_mode];
	    for (auto & timer_num: free_) {
		free_.pop_front();
		return timer_num;
	    }
	    throw std::range_error("LEDC::Timer::free empty");
	}(),
	freq_hz
	    ? freq_hz
	    : APB_CLK_FREQ / (1 << duty_resolution),
    }
{
    ESP_ERROR_CHECK(ledc_timer_config(&config));
}

Timer::~Timer() {
    ESP_ERROR_CHECK(ledc_timer_rst(config.speed_mode, config.timer_num));
    free[config.speed_mode].push_front(config.timer_num);
}

void Timer::set_freq(uint32_t freq_hz) {
    ESP_ERROR_CHECK(ledc_set_freq(config.speed_mode, config.timer_num, freq_hz));
}

uint32_t Timer::get_freq() {
    return ledc_get_freq(config.speed_mode, config.timer_num);
}

void Timer::pause() {
    ESP_ERROR_CHECK(ledc_timer_pause(config.speed_mode, config.timer_num));
}

void Timer::resume() {
    ESP_ERROR_CHECK(ledc_timer_resume(config.speed_mode, config.timer_num));
}

std::forward_list<ledc_channel_t> Channel::free[] {
    {
	LEDC_CHANNEL_0,
	LEDC_CHANNEL_1,
	LEDC_CHANNEL_2,
	LEDC_CHANNEL_3,
	LEDC_CHANNEL_4,
	LEDC_CHANNEL_5,
	LEDC_CHANNEL_6,
	LEDC_CHANNEL_7,
    },
    {
	LEDC_CHANNEL_0,
	LEDC_CHANNEL_1,
	LEDC_CHANNEL_2,
	LEDC_CHANNEL_3,
	LEDC_CHANNEL_4,
	LEDC_CHANNEL_5,
	LEDC_CHANNEL_6,
	LEDC_CHANNEL_7,
    },
};

Channel::Channel(
    Timer &		timer_,
    int			gpio_num,
    ledc_intr_type_t	intr_type,
    uint32_t		duty,
    int			hpoint)
:
    timer(timer_),
    config {
	gpio_num,
	timer.config.speed_mode,
	[this](){
	    auto free_ = free[timer.config.speed_mode];
	    for (auto & channel: free_) {
		free_.pop_front();
		return channel;
	    }
	    throw std::range_error("LEDC::Channel::free empty");
	}(),
	intr_type,
	timer.config.timer_num,
	duty,
	hpoint,
    }
{
    ESP_ERROR_CHECK(ledc_channel_config(&config));
}

Channel::~Channel() {
    free[timer.config.speed_mode].push_front(config.channel);
}


void Channel::update_duty() {
    ESP_ERROR_CHECK(ledc_update_duty(config.speed_mode, config.channel));
}

void Channel::stop(uint32_t idle_level) {
    ESP_ERROR_CHECK(ledc_stop(config.speed_mode, config.channel, idle_level));
}

void Channel::set_duty_with_hpoint(uint32_t duty, uint32_t hpoint) {
    ESP_ERROR_CHECK(ledc_set_duty_with_hpoint(
	config.speed_mode, config.channel, duty, hpoint));
}

int Channel::get_hpoint() {
    return ledc_get_hpoint(config.speed_mode, config.channel);
}

void Channel::set_duty(uint32_t duty) {
    ESP_ERROR_CHECK(ledc_set_duty(config.speed_mode, config.channel, duty));
}

uint32_t Channel::get_duty() {
    return ledc_get_duty(config.speed_mode, config.channel);
}

void Channel::set_fade(
    uint32_t duty, ledc_duty_direction_t fade_direction, uint32_t step_num,
    uint32_t duty_cycle_num, uint32_t duty_scale)
{
    ESP_ERROR_CHECK(ledc_set_fade(config.speed_mode, config.channel,
	duty, fade_direction, step_num, duty_cycle_num, duty_scale));
}

void Channel::set_fade_with_step(
    uint32_t target_duty, uint32_t scale, uint32_t cycle_num)
{
    ESP_ERROR_CHECK(ledc_set_fade_with_step(config.speed_mode, config.channel,
	target_duty, scale, cycle_num));
}

void Channel::set_fade_with_time(uint32_t target_duty, int max_fade_time_ms) {
    ESP_ERROR_CHECK(ledc_set_fade_with_time(config.speed_mode, config.channel,
	target_duty, max_fade_time_ms));
}

void Channel::fade_start(ledc_fade_mode_t fade_mode) {
    ESP_ERROR_CHECK(ledc_fade_start(config.speed_mode, config.channel,
	fade_mode));
}

void Channel::set_duty_and_update(uint32_t duty, uint32_t hpoint) {
    ESP_ERROR_CHECK(ledc_set_duty_and_update(config.speed_mode, config.channel,
	duty, hpoint));
}

void Channel::set_fade_time_and_start(
    uint32_t target_duty, uint32_t max_fade_time_ms, ledc_fade_mode_t fade_mode)
{
    ESP_ERROR_CHECK(ledc_set_fade_time_and_start(
	config.speed_mode, config.channel,
	target_duty, max_fade_time_ms, fade_mode));
}

void Channel::set_fade_step_and_start(
    uint32_t target_duty, uint32_t scale, uint32_t cycle_num,
    ledc_fade_mode_t fade_mode)
{
    ESP_ERROR_CHECK(ledc_set_fade_step_and_start(
	config.speed_mode, config.channel,
	target_duty, scale, cycle_num, fade_mode));
}

}
