#include <algorithm>

#include "PCA9685.h"

namespace { 

struct RegisterKey {
    enum : uint8_t {
	mode1		= 0x00,
	mode2		= 0x01,

	subadr1		= 0x02,
	subadr2		= 0x03,
	subadr3		= 0x04,
	allcalladr		= 0x05,

	pwm0_on_l		= 0x06,
	pwm0_on_h		= 0x07,
	pwm0_off_l		= 0x08,
	pwm0_off_h		= 0x09,

	pwm1_on_l		= 0x0a,
	pwm1_on_h		= 0x0b,
	pwm1_off_l		= 0x0c,
	pwm1_off_h		= 0x0d,

	pwm2_on_l		= 0x0e,
	pwm2_on_h		= 0x0f,
	pwm2_off_l		= 0x10,
	pwm2_off_h		= 0x11,

	pwm3_on_l		= 0x12,
	pwm3_on_h		= 0x13,
	pwm3_off_l		= 0x14,
	pwm3_off_h		= 0x15,

	pwm4_on_l		= 0x16,
	pwm4_on_h		= 0x17,
	pwm4_off_l		= 0x18,
	pwm4_off_h		= 0x19,

	pwm5_on_l		= 0x1a,
	pwm5_on_h		= 0x1b,
	pwm5_off_l		= 0x1c,
	pwm5_off_h		= 0x1d,

	pwm6_on_l		= 0x1e,
	pwm6_on_h		= 0x1f,
	pwm6_off_l		= 0x20,
	pwm6_off_h		= 0x21,

	pwm7_on_l		= 0x22,
	pwm7_on_h		= 0x23,
	pwm7_off_l		= 0x24,
	pwm7_off_h		= 0x25,

	pwm8_on_l		= 0x26,
	pwm8_on_h		= 0x27,
	pwm8_off_l		= 0x28,
	pwm8_off_h		= 0x29,

	pwm9_on_l		= 0x2a,
	pwm9_on_h		= 0x2b,
	pwm9_off_l		= 0x2c,
	pwm9_off_h		= 0x2d,

	pwm10_on_l		= 0x2e,
	pwm10_on_h		= 0x2f,
	pwm10_off_l		= 0x30,
	pwm10_off_h		= 0x31,

	pwm11_on_l		= 0x32,
	pwm11_on_h		= 0x33,
	pwm11_off_l		= 0x34,
	pwm11_off_h		= 0x35,

	pwm12_on_l		= 0x36,
	pwm12_on_h		= 0x37,
	pwm12_off_l		= 0x38,
	pwm12_off_h		= 0x39,

	pwm13_on_l		= 0x3a,
	pwm13_on_h		= 0x3b,
	pwm13_off_l		= 0x3c,
	pwm13_off_h		= 0x3d,

	pwm14_on_l		= 0x3e,
	pwm14_on_h		= 0x3f,
	pwm14_off_l		= 0x40,
	pwm14_off_h		= 0x41,

	pwm15_on_l		= 0x42,
	pwm15_on_h		= 0x43,
	pwm15_off_l		= 0x44,
	pwm15_off_h		= 0x45,

	// ... reserved ...

	all_pwm_on_l	= 0xfa,
	all_pwm_on_h	= 0xfb,
	all_pwm_off_l	= 0xfc,
	all_pwm_off_h	= 0xfd,

	pre_scale		= 0xfe,
	test_mode		= 0xff,
    };
};

}

PCA9685::Mode::Mode(
    bool	allcall_,
    bool	sub3_,
    bool	sub2_,
    bool	sub1_,
    bool	sleep_,
    bool	ai_,
    bool	extclk_,
    bool	restart_,
    uint8_t	outne_,
    bool	outdrv_,
    bool	och_,
    bool	invrt_)
:
#if BYTE_ORDER == BIG_ENDIAN
    invrt	(invrt_),
    och		(och_),
    outdrv	(outdrv_),
    outne	(outne_),
    restart	(restart_),
    extclk	(extclk_),
    ai		(ai_),
    sleep	(sleep_),
    sub1	(sub1_),
    sub2	(sub2_),
    sub3	(sub3_),
    allcall	(allcall_),
#else
    allcall	(allcall_),
    sub3	(sub3_),
    sub2	(sub2_),
    sub1	(sub1_),
    sleep	(sleep_),
    ai		(ai_),
    extclk	(extclk_),
    restart	(restart_),
    outne	(outne_),
    outdrv	(outdrv_),
    och		(och_),
    invrt	(invrt_)
#endif
{}

PCA9685::Pwm::Pwm()
:
#if BYTE_ORDER == BIG_ENDIAN
    offFull	(1),
    off		(0),
    onFull	(0),
    on		(0)
#else
    on		(0),
    onFull	(0),
    off		(0),
    offFull	(1)
#endif
{}

uint8_t PCA9685::addressOf(unsigned index) {
    return 0b1000000 | (0b0111111 & index);
}

PCA9685::PCA9685(
    I2C::Master const *	i2cMaster_,
    uint8_t		address_)
:
    i2cMaster		(i2cMaster_),
    address		(address_)
{}

PCA9685::PCA9685(
    I2C::Master const *	i2cMaster_,
    uint8_t		address_,
    Mode		mode)
:
    PCA9685 {i2cMaster_, address_}
{
    setMode(mode);
}

// how long we should wait for commands to complete
static TickType_t constexpr wait = 1;

uint8_t PCA9685::getRegister(uint8_t key) const {
    uint8_t value;
    {
	i2cMaster->commands(address, wait)
	    .writeByte(key)
	    .startRead()
	    .readByte(&value);
    }
    return value;
}

void PCA9685::setRegister(uint8_t key, uint8_t value) const {
    i2cMaster->commands(address, wait)
	.writeByte(key)
	.writeByte(value);
}

void PCA9685::setRegisters(uint8_t key, uint8_t const * value, size_t count, bool ai) const {
    if (ai) {
	i2cMaster->commands(address, wait)
	    .writeByte(key)
	    .writeBytes(value, count);
    } else {
	for (; count--;) {
	    setRegister(key++, *value++);
	}
    }
}

template <typename T>
T PCA9685::getRegisters(uint8_t key, bool ai) const {
    T value;
    if (ai) {
	i2cMaster->commands(address, wait)
	    .writeByte(key)
	    .startRead()
	    .readBytes(&value, sizeof value);
    } else {
	uint8_t (&values)[sizeof value]
	    {reinterpret_cast<uint8_t (&)[sizeof value]>(value)};
	for (auto & e: values) {
	    e = getRegister(key++);
	}
    }
    return value;
}

template <typename T>
void PCA9685::setRegisters(uint8_t key, T value, bool ai) const {
    setRegisters(key, reinterpret_cast<uint8_t *>(&value), sizeof value, ai);
}

PCA9685::Mode PCA9685::getMode(bool ai) const {
    uint16_t value {getRegisters<uint16_t>(RegisterKey::mode1, ai)};
    return *reinterpret_cast<Mode *>(&value);
}

void PCA9685::setMode(Mode value, bool ai) const {
    setRegisters<uint16_t>(RegisterKey::mode1,
	*reinterpret_cast<uint16_t *>(&value), ai);
}

uint8_t PCA9685::getSoftAddress(uint8_t key) const {
    return getRegister(RegisterKey::subadr1 + key);
}

void PCA9685::setSoftAddress(uint8_t key, uint8_t value) const {
    setRegister(RegisterKey::subadr1 + key, value);
}

static uint8_t pwmKeyFrom(size_t index) {
    return ~0 == index
	? RegisterKey::all_pwm_on_l
	: RegisterKey::pwm0_on_l
	    + static_cast<uint8_t>((0xf & index) * sizeof(PCA9685::Pwm));
}

size_t constexpr PCA9685::pwmCount;

PCA9685::Pwm PCA9685::getPwm(size_t index, bool ai) const {
    uint32_t value {getRegisters<uint32_t>(pwmKeyFrom(index), ai)};
    return *reinterpret_cast<Pwm *>(&value);
}

void PCA9685::setPwm(size_t index, Pwm value, bool ai) const {
    setRegisters<uint32_t>(pwmKeyFrom(index),
	*reinterpret_cast<uint32_t *>(&value),
	ai);
}

void PCA9685::setPwms(size_t index, Pwm const * value, size_t count, bool ai) const {
    index = std::min(index, pwmCount - 1);
    count = std::min(count, pwmCount);
    setRegisters(pwmKeyFrom(index), reinterpret_cast<uint8_t const *>(value),
	std::min(count, count - index) * sizeof *value, ai);
}

uint8_t PCA9685::getPreScale() const {
    return getRegister(RegisterKey::pre_scale);
}

void PCA9685::setPreScale(uint8_t value) const {
    setRegister(RegisterKey::pre_scale, value);
}

PCA9685::~PCA9685() {
}
