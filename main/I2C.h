#pragma once

#include <cstdint>
#include <cstdlib>

#include <freertos/FreeRTOS.h>

#include <driver/i2c.h>

namespace I2C {

/// An I2C::Config is an i2c_config_t
/// with convenience setter methods that can be chained together.
/// The result can be used to construct an I2C::Driver
/// for an I2C::Master or I2C::Slave.
struct Config : public i2c_config_t {
public:
    Config();
    #define setter(name) Config & name##_(decltype(name) s);
    setter(mode)
    setter(sda_io_num)
    setter(sda_pullup_en)
    setter(scl_io_num)
    setter(scl_pullup_en)
    #undef setter
    #define setter(name) Config & master_##name##_(decltype(master.name) s);
    setter(clk_speed)
    #undef setter
    #define setter(name) Config & slave_##name##_(decltype(slave.name) s);
    setter(addr_10bit_en)
    setter(slave_addr)
    #undef setter
};

/// An I2C::Driver has a port that is used by its I2C::Master or I2C::Slave
class Driver {
private:
    i2c_port_t const port;
protected:
    Driver(
	Config &	config,
	i2c_port_t	port,
	size_t		slaveModeReceiveBufferLength,
	size_t		slaveModeTransmitBufferLength,
	int		interruptAllocationFlags);
public:
    operator i2c_port_t() const;
    ~Driver();
};

/// An I2C::Master is an I2C::Driver used to communicate as an I2C master
/// with I2C::Master::Commands
class Master : public Driver {
public:

    /// An I2C::Master::Commands is constructed for/by I2C::Master::commands()
    /// to command an I2C slave.
    /// The commands are configured by chaining together its Commands methods.
    /// Communication occurs during Commands destruction.
    class Commands {
    private:
	Master const &		master;
	uint8_t const		address;
	TickType_t const	wait;
	i2c_cmd_handle_t	command;

	operator i2c_cmd_handle_t() const;

    public:
	Commands(
	    Master const &	master,
	    uint8_t		address,
	    TickType_t		wait	= portMAX_DELAY,
	    bool		read	= false,
	    bool		ack	= true);

	Commands & start(bool read = false, bool ack = true);

	Commands & startRead(bool ack = true);

	Commands & writeByte(uint8_t data, bool ack = true);

	Commands & writeBytes(void const * data, size_t size, bool ack = true);

	Commands & readByte(uint8_t * data,
	    i2c_ack_type_t ack = I2C_MASTER_NACK);

	Commands & readBytes(void * data, size_t size,
	    i2c_ack_type_t ack = I2C_MASTER_LAST_NACK);

	~Commands() noexcept(false);
    };

    Master(
	Config &	config,
	i2c_port_t	port,
	int		interuptAllocationFlags);

    Commands commands(
	uint8_t		address,
	TickType_t	wait	= portMAX_DELAY,
	bool		read	= false,
	bool		ack	= true) const;
};

/// An I2C::Master is an I2C::Driver used to communicate as an I2C slave.
class Slave : public Driver {
public:
    Slave(
	Config &	config,
	i2c_port_t	port,
	size_t		slaveModeReceiveBufferLength,
	size_t		slaveModeTransmitBufferLength,
	int		interuptAllocationFlags);

    size_t writeBytes(uint8_t * data, size_t size, TickType_t wait);

    size_t readBytes(uint8_t * data, size_t size, TickType_t wait);
};

}
