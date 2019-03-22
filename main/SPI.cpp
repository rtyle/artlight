#include <cassert>
#include <cstring>

#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>

#include <esp_err.h>

#include "SPI.h"

namespace SPI {

/* static */ spi_bus_config_t const Bus::NoConfig {
    -1,	// mosi_io_num
    -1,	// miso_io_num
    -1,	// sclk_io_num
    -1,	// quadwp_io_num
    -1,	// quadhd_io_num
     0,	// max_transfer_sz (0 => 4094)
     0,	// flags
     0,	// intr_flags
};

/* static */ spi_bus_config_t const Bus::HspiConfig {
    13,	// mosi_io_num
    12,	// miso_io_num
    14,	// sclk_io_num
     2,	// quadwp_io_num
     4,	// quadhd_io_num
     0,	// max_transfer_sz (0 => 4094)
     0,	// flags
     0,	// intr_flags
};

/* static */ spi_bus_config_t const Bus::VspiConfig {
    23,	// mosi_io_num
    19,	// miso_io_num
    18,	// sclk_io_num
    22,	// quadwp_io_num
    21,	// quadhd_io_num
     0,	// max_transfer_sz (0 => 4094)
     0,	// flags
     0,	// intr_flags
};

Bus::Config::Config(spi_bus_config_t const & that) : spi_bus_config_t(that) {}
Bus::Config::Config() : Config(NoConfig) {}
#define setter(name) Bus::Config & Bus::Config::name##_(decltype(name) s) \
    {name = s; return *this;}
setter(mosi_io_num)
setter(miso_io_num)
setter(sclk_io_num)
setter(quadwp_io_num)
setter(quadhd_io_num)
setter(max_transfer_sz)
setter(flags)
setter(intr_flags)
#undef setter

Bus::Bus(
    spi_host_device_t	host_,
    Config const &	config,
    int			dmaChannel_)
:
    host	(host_),
    dmaChannel	(dmaChannel_)
{
    ESP_ERROR_CHECK(spi_bus_initialize(host, &config, dmaChannel));
}

Bus::operator spi_host_device_t() const {return host;}

Bus::~Bus() {
    ESP_ERROR_CHECK(spi_bus_free(host));
}

Device::Config::Config() : spi_device_interface_config_t {} {}
#define setter(name) Device::Config & Device::Config::name##_(decltype(name) s) \
    {name = s; return *this;}
setter(command_bits)
setter(address_bits)
setter(dummy_bits)
setter(mode)
setter(duty_cycle_pos)
setter(cs_ena_pretrans)
setter(cs_ena_posttrans)
setter(clock_speed_hz)
setter(input_delay_ns)
setter(spics_io_num)
setter(flags)
setter(queue_size)
setter(pre_cb)
setter(post_cb)
#undef setter

Device::Device (
    Bus const *		bus_,
    Config const &	config)
:
    bus(bus_),
    handle(nullptr)
{
    ESP_ERROR_CHECK(spi_bus_add_device(*bus, &config, &handle));
}

Device::operator spi_device_handle_t() const {return handle;}

Device::~Device() {
    ESP_ERROR_CHECK(spi_bus_remove_device(handle));
}

Transaction::Config::Config() : spi_transaction_t {} {}
#define setter(name) \
    Transaction::Config & Transaction::Config::name##_(decltype(name) s) \
	{name = s; return *this;}
setter(flags)
setter(cmd)
setter(addr)
setter(length)
setter(rxlength)
setter(user)
setter(tx_buffer)
setter(rx_buffer)
#undef setter
#define setter(name) Transaction::Config & Transaction::Config::name##_( \
    decltype(name) s, size_t z) { \
	std::memcpy(name, s, sizeof name < z ? sizeof name : z); return *this;}
setter(tx_data)
setter(rx_data)
#undef setter

Transaction::Transaction(
    Device const &	device_,
    Config &		config_)
:
    device(device_),
    config(config_)
{
    ESP_ERROR_CHECK(
	spi_device_queue_trans(device, &config, portMAX_DELAY));
}

Transaction::~Transaction() {
    spi_transaction_t * result;
    ESP_ERROR_CHECK(
	spi_device_get_trans_result(device, &result, portMAX_DELAY));
    assert(result == &config);
}

}
