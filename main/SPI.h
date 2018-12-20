#ifndef SPI_h__
#define SPI_h__

#include <driver/spi_common.h>
#include <driver/spi_master.h>

namespace SPI {

class Device;

class Bus {
private:
    spi_host_device_t const host;
    int const dmaChannel;

public:
    static spi_bus_config_t const NoConfig;	///< no pins
    static spi_bus_config_t const HspiConfig;	///< HSPI IOMUX pins
    static spi_bus_config_t const VspiConfig;	///< VSPI IOMUX pins
    struct Config : public spi_bus_config_t {
    public:
	Config(spi_bus_config_t const & that);
	Config();
	#define setter(name) Config & name##_(decltype(name));
	setter(mosi_io_num)
	setter(miso_io_num)
	setter(sclk_io_num)
	setter(quadwp_io_num)
	setter(quadhd_io_num)
	setter(max_transfer_sz)
	setter(flags)
	setter(intr_flags)
	#undef setter
    };

    Bus(
	spi_host_device_t	host_,		// HSPI_HOST or VSPI_HOST
	Config const &		config,
	int			dmaChannel_	= 0);	// none, 1 or 2

    operator spi_host_device_t() const;

    ~Bus();
};

class Device {
private:
    Bus const * bus;
    spi_device_handle_t handle;

public:
    struct Config : public spi_device_interface_config_t {
	Config();
	#define setter(name) Config & name##_(decltype(name));
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
    };

    Device (
	Bus const *		bus_,
	Config const &		config);

    operator spi_device_handle_t() const;

    ~Device();
};

class Transaction {
public:
    struct Config : public spi_transaction_t {
    public:
	Config();
	#define setter(name) Config & name##_(decltype(name));
	setter(flags)
	setter(cmd)
	setter(addr)
	setter(length)
	setter(rxlength)
	setter(user)
	setter(tx_buffer)
	setter(rx_buffer)
	#undef setter
	#define setter(name) Config & name##_(decltype(name), size_t);
	setter(tx_data)
	setter(rx_data)
	#undef setter
    };

private:
    Device const & device;
    Config config;

public:
    Transaction(
	Device const &	device_,
	Config &	config_);

    ~Transaction();
};

}

#endif
