#pragma once

#include <cstdint>

#include <machine/endian.h>

#include "I2C.h"

class PCA9685 {
private:
    I2C::Master const * const i2cMaster;
    uint8_t const	address;

    uint8_t getRegister(uint8_t key) const;
    void setRegister(uint8_t key, uint8_t value) const;

    void setRegisters(uint8_t key, uint8_t const * value, size_t count, bool ai = false) const;

    template <typename T>
    T getRegisters(uint8_t key, bool ai = false) const;
    template <typename T>
    void setRegisters(uint8_t key, T value, bool ai = false) const;

public:
    struct Mode {
	#if BYTE_ORDER == BIG_ENDIAN
	    uint16_t		:3;
	    uint16_t	invrt	:1;
	    uint16_t	och	:1;
	    uint16_t	outdrv	:1;
	    uint16_t	outne	:2;
	    uint16_t	restart	:1;
	    uint16_t	extclk	:1;
	    uint16_t	ai	:1;
	    uint16_t	sleep	:1;
	    uint16_t	sub1	:1;
	    uint16_t	sub2	:1;
	    uint16_t	sub3	:1;
	    uint16_t	allcall	:1;
	#else
	    uint16_t	allcall	:1;
	    uint16_t	sub3	:1;
	    uint16_t	sub2	:1;
	    uint16_t	sub1	:1;
	    uint16_t	sleep	:1;
	    uint16_t	ai	:1;
	    uint16_t	extclk	:1;
	    uint16_t	restart	:1;
	    uint16_t	outne	:2;
	    uint16_t	outdrv	:1;
	    uint16_t	och	:1;
	    uint16_t	invrt	:1;
	    uint16_t		:3;
	#endif
	Mode(
	    bool	allcall,
	    bool	sub3,
	    bool	sub2,
	    bool	sub1,
	    bool	sleep,
	    bool	ai,
	    bool	extclk,
	    bool	restart,
	    uint8_t	outne,
	    bool	outdrv,
	    bool	och,
	    bool	invrt);
    };

    struct Pwm {
	#if BYTE_ORDER == BIG_ENDIAN
	    uint16_t		:3;
	    uint16_t	onFull	:1;
	    uint16_t	on	:12;
	    uint16_t		:3;
	    uint16_t	offFull	:1;
	    uint16_t	off	:12;
	#else
	    uint16_t	on	:12;
	    uint16_t	onFull	:1;
	    uint16_t		:3;
	    uint16_t	off	:12;
	    uint16_t	offFull	:1;
	    uint16_t		:3;
	#endif
    };

    struct SoftAddressKey {
	enum : uint8_t {
	    subadr1	= 0,
	    subadr2 	= 1,
	    subadr3 	= 2,
	    allcalladr	= 3,
	};
    };

    // convert A5-A0 configured on chip to I2C address
    static uint8_t addressOf(unsigned index);

    PCA9685(
	I2C::Master const *	i2cMaster,
	uint8_t			address);

    PCA9685(
	I2C::Master const *	i2cMaster,
	uint8_t			address,
	Mode			mode);

    PCA9685::Mode getMode(bool ai = false) const;
    void setMode(Mode value, bool ai = false) const;

    uint8_t getSoftAddress(uint8_t) const;
    void setSoftAddress(uint8_t key, uint8_t value) const;

    static size_t constexpr pwmCount {16};

    Pwm getPwm(size_t index, bool ai = false) const;
    void setPwm(size_t index, Pwm value, bool ai = false) const;
    void setPwms(size_t index, Pwm const * value, size_t count, bool ai = false) const;

    uint8_t getPreScale() const;
    void setPreScale(uint8_t value) const;

    virtual ~PCA9685();
};
