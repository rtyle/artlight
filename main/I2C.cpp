#include <esp_log.h>

#include "Error.h"
#include "I2C.h"

namespace I2C {

Config::Config() : i2c_config_t {} {}
#define setter(name) Config & Config::name##_(decltype(name) s) \
    {name = s; return *this;}
setter(mode)
setter(sda_io_num)
setter(sda_pullup_en)
setter(scl_io_num)
setter(scl_pullup_en)
#undef setter
#define setter(name) Config & Config::master_##name##_(decltype(master.name) s) \
    {master.name = s; return *this;}
setter(clk_speed)
#undef setter
#define setter(name) Config & Config::slave_##name##_(decltype(slave.name) s) \
    {slave.name = s; return *this;}
setter(addr_10bit_en)
setter(slave_addr)
#undef setter

Driver::Driver(
    Config &	config,
    i2c_port_t	port_,
    size_t	slaveModeReceiveBufferLength,
    size_t	slaveModeTransmitBufferLength,
    int		interruptAllocationFlags)
:
    port(port_)
{
    Error::throwIf(i2c_param_config(port, &config));
    Error::throwIf(i2c_driver_install(
	port,
	config.mode,
	slaveModeReceiveBufferLength,
	slaveModeTransmitBufferLength,
	interruptAllocationFlags));
}

Driver::operator i2c_port_t() const {return port;}

Driver::~Driver() {
    Error::throwIf(i2c_driver_delete(port));
}

Master::Commands::operator i2c_cmd_handle_t() const {return command;}

Master::Commands::Commands(
    Master const &	master_,
    uint8_t		address_,
    TickType_t		wait_,
    bool		read,
    bool		ack)
:
    master	(master_),
    address	(address_),
    wait	(wait_),
    command	(i2c_cmd_link_create())
{
    start(read, ack);
}

Master::Commands & Master::Commands::start(bool read, bool ack) {
    Error::throwIf(i2c_master_start(*this));
    writeByte(address << 1 | read, ack);
    return *this;
}

Master::Commands & Master::Commands::startRead(bool ack) {
    return start(true, ack);
}

Master::Commands & Master::Commands::writeByte(
	uint8_t data, bool ack) {
    Error::throwIf(i2c_master_write_byte(*this, data, ack));
    return *this;
}

Master::Commands & Master::Commands::writeBytes(
	void * data, size_t size, bool ack) {
    Error::throwIf(i2c_master_write(
	*this, static_cast<uint8_t *>(data), size, ack));
    return *this;
}

Master::Commands & Master::Commands::readByte(
	uint8_t * data, i2c_ack_type_t ack) {
    Error::throwIf(i2c_master_read_byte(*this, data, ack));
    return *this;
}

Master::Commands & Master::Commands::readBytes(
	void * data, size_t size, i2c_ack_type_t ack) {
    Error::throwIf(i2c_master_read(
	*this, static_cast<uint8_t *>(data), size, ack));
    return *this;
}

Master::Commands::~Commands() noexcept(false) {
    try {
	Error::throwIf(i2c_master_stop(*this));
	Error::throwIf(i2c_master_cmd_begin(master, *this, wait));
	i2c_cmd_link_delete(*this);
    } catch (...) {
	i2c_cmd_link_delete(*this);
	throw;
    }
}

Master::Master(
    Config &	config,
    i2c_port_t	port,
    int		interuptAllocationFlags)
:
    Driver(
	config.mode_(I2C_MODE_MASTER),
	port,
	0,
	0,
	interuptAllocationFlags)
{}

Master::Commands Master::commands(
    uint8_t	address,
    TickType_t	wait,
    bool	read,
    bool	ack)
const
{
    return Commands(*this, address, wait, read, ack);
}

Slave::Slave(
    Config &	config,
    i2c_port_t	port,
    size_t	slaveModeReceiveBufferLength,
    size_t	slaveModeTransmitBufferLength,
    int		interuptAllocationFlags)
:
    Driver(
	config.mode_(I2C_MODE_SLAVE),
	port,
	slaveModeReceiveBufferLength,
	slaveModeTransmitBufferLength,
	interuptAllocationFlags)
{}

size_t Slave::writeBytes(uint8_t * data, size_t size, TickType_t wait) {
    return Error::throwIfIs(i2c_slave_write_buffer(*this, data, size, wait));
}

size_t Slave::readBytes(uint8_t * data, size_t size, TickType_t wait) {
    return Error::throwIfIs(i2c_slave_read_buffer(*this, data, size, wait));
}

}
