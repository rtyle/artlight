#include "LEDC.h"

namespace LEDC {

ISR::ISR(int intr_alloc_flags) {
    ESP_ERROR_CHECK(ledc_fade_func_install(intr_alloc_flags));
}

ISR::~ISR() {
    ledc_fade_func_uninstall();
}

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
    ledc_mode_t		speed_mode_,
    ledc_timer_bit_t	duty_resolution,
    uint32_t		freq_hz)
:
    speed_mode	(speed_mode_),
    timer_num	(
	[this](){
	    auto free_ = free[speed_mode];
	    for (auto & timer_num: free_) {
		free_.pop_front();
		return timer_num;
	    }
	    throw std::bad_alloc();
	}())
{
    ledc_timer_config_t config {
	speed_mode,
	duty_resolution,
	timer_num,
	freq_hz
	    ? freq_hz
	    : APB_CLK_FREQ / (1 << duty_resolution),
    };
    ESP_ERROR_CHECK(ledc_timer_config(&config));
}

Timer::~Timer() {
    free[speed_mode].push_front(timer_num);
}

void Timer::set_freq(uint32_t freq_hz) {
    ESP_ERROR_CHECK(ledc_set_freq(speed_mode, timer_num, freq_hz));
}

uint32_t Timer::get_freq() {
    return ledc_get_freq(speed_mode, timer_num);
}

void Timer::rst() {
    ESP_ERROR_CHECK(ledc_timer_rst(speed_mode, timer_num));
}

void Timer::pause() {
    ESP_ERROR_CHECK(ledc_timer_pause(speed_mode, timer_num));
}

void Timer::resume() {
    ESP_ERROR_CHECK(ledc_timer_resume(speed_mode, timer_num));
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
    Timer const &	timer,
    int			gpio_num,
    uint32_t		duty,
    int			hpoint,
    ledc_intr_type_t	intr_type)
:
    speed_mode(timer.speed_mode),
    channel([this](){
	    auto free_ = free[speed_mode];
	    for (auto & channel: free_) {
		free_.pop_front();
		return channel;
	    }
	    throw std::bad_alloc();
	}())
{
    ledc_channel_config_t config {
	gpio_num,
	speed_mode,
	channel,
	intr_type,
	timer.timer_num,
	duty,
	hpoint,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&config));
}

Channel::~Channel() {
    free[speed_mode].push_front(channel);
}

void Channel::bind_timer(Timer const & timer) {
    ESP_ERROR_CHECK(ledc_bind_channel_timer(speed_mode, channel,
	timer.timer_num));
}

void Channel::update_duty() {
    ESP_ERROR_CHECK(ledc_update_duty(speed_mode, channel));
}

void Channel::stop(uint32_t idle_level) {
    ESP_ERROR_CHECK(ledc_stop(speed_mode, channel, idle_level));
}

void Channel::set_duty_with_hpoint(uint32_t duty, uint32_t hpoint) {
    ESP_ERROR_CHECK(ledc_set_duty_with_hpoint(speed_mode, channel,
	duty, hpoint));
}

int Channel::get_hpoint() {
    return ledc_get_hpoint(speed_mode, channel);
}

void Channel::set_duty(uint32_t duty) {
    ESP_ERROR_CHECK(ledc_set_duty(speed_mode, channel, duty));
}

uint32_t Channel::get_duty() {
    return ledc_get_duty(speed_mode, channel);
}

void Channel::set_fade(
    uint32_t duty, ledc_duty_direction_t fade_direction, uint32_t step_num,
    uint32_t duty_cycle_num, uint32_t duty_scale)
{
    ESP_ERROR_CHECK(ledc_set_fade(speed_mode, channel,
	duty, fade_direction, step_num, duty_cycle_num, duty_scale));
}

void Channel::set_fade_with_step(
    uint32_t target_duty, uint32_t scale, uint32_t cycle_num)
{
    ESP_ERROR_CHECK(ledc_set_fade_with_step(speed_mode, channel,
	target_duty, scale, cycle_num));
}

void Channel::set_fade_with_time(uint32_t target_duty, int max_fade_time_ms) {
    ESP_ERROR_CHECK(ledc_set_fade_with_time(speed_mode, channel,
	target_duty, max_fade_time_ms));
}

void Channel::fade_start(ledc_fade_mode_t fade_mode) {
    ESP_ERROR_CHECK(ledc_fade_start(speed_mode, channel, fade_mode));
}

void Channel::set_duty_and_update(uint32_t duty, uint32_t hpoint) {
    ESP_ERROR_CHECK(ledc_set_duty_and_update(speed_mode, channel,
	duty, hpoint));
}

void Channel::set_fade_time_and_start(
    uint32_t target_duty, uint32_t max_fade_time_ms, ledc_fade_mode_t fade_mode)
{
    ESP_ERROR_CHECK(ledc_set_fade_time_and_start(speed_mode, channel,
	target_duty, max_fade_time_ms, fade_mode));
}

void Channel::set_fade_step_and_start(
    uint32_t target_duty, uint32_t scale, uint32_t cycle_num,
    ledc_fade_mode_t fade_mode)
{
    ESP_ERROR_CHECK(ledc_set_fade_step_and_start(speed_mode, channel,
	target_duty, scale, cycle_num, fade_mode));
}

}
