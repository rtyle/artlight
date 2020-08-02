#include <cinttypes>

#include <esp_log.h>

#include "HT7M2xxxMotionSensor.h"

// symmetric encode/decode for 16 bit value
static uint16_t translate(uint16_t value) {
    return
#if BYTE_ORDER == BIG_ENDIAN
    value;
#else
    __builtin_bswap16(value);
#endif
}

// scope of everything in this anonymous namespace
// is limited to this translation unit and has internal linkage
namespace {

struct RegisterKey {
    enum : uint8_t {
	configuration0			= 0x00,
	configuration1			= 0x01,
	configuration2			= 0x02,
	triggerTimeInterval		= 0x03,
	eepromAccess			= 0x04,
	pirRawData			= 0x05,
	opticalSensorRawData		= 0x06,
	temperatureSensorRawData	= 0x07,
	status				= 0x08,
	manufacturerId			= 0x09,
	firmwareVersion			= 0x0a,
    };
};

}

HT7M2xxxMotionSensor::HT7M2xxxMotionSensor(
    asio::io_context &	io_,
    I2C::Master const *	i2cMaster_)
:
    MotionSensor	{"HT7M2xxx", io_},
    i2cMaster		{i2cMaster_},
    pirRawData		{0x800}
{
    assertId();
    timer.start();
}

// how long we should wait for commands to complete
static TickType_t constexpr wait = 1;

uint16_t HT7M2xxxMotionSensor::getRegister(uint8_t key) const {
    uint16_t value;
    {
	i2cMaster->commands(address, wait)
	    .writeByte(key)
	    .startRead()
	    .readBytes(&value, sizeof value);
    }
    return value;
}

void HT7M2xxxMotionSensor::setRegister(uint8_t key, uint16_t value) const {
    i2cMaster->commands(address, wait)
	.writeByte(key)
	.writeBytes(&value, sizeof value);
}

void HT7M2xxxMotionSensor::assertId() const {
    assert(0x04d9 == translate(getRegister(RegisterKey::manufacturerId)));
}

HT7M2xxxMotionSensor::Configuration0::Configuration0(
    uint8_t	standbyPeriod_,
    uint8_t	temperatureCelsius_)
:
#if BYTE_ORDER == BIG_ENDIAN
    temperatureCelsius	{temperatureCelsius_},
    standbyPeriod	{standbyPeriod_}
#else
    standbyPeriod	{standbyPeriod_},
    temperatureCelsius	{temperatureCelsius_}
#endif
{}

HT7M2xxxMotionSensor::Configuration0
HT7M2xxxMotionSensor::getConfiguration0() const {
    uint16_t value = getRegister(RegisterKey::configuration0);
    return *reinterpret_cast<Configuration0 *>(&value);
}

void HT7M2xxxMotionSensor::setConfiguration0(Configuration0 value) const {
    setRegister(RegisterKey::configuration0, *reinterpret_cast<uint16_t *>(&value));
}

HT7M2xxxMotionSensor::Configuration1::Configuration1(
    uint8_t	gain_,
    uint8_t	pirThreshold_,
    bool	actEnable_,
    bool	retrigger_,
    bool	pirEnable_,
    bool	lvdEnable_,
    uint8_t	lvd_)
:
#if BYTE_ORDER == BIG_ENDIAN
    lvd		{lvd_},
    lvdEnable	{lvdEnable_},
    pirEnable	{pirEnable_},
    retrigger	{retrigger_},
    actEnable	{actEnable_},
    pirThreshold{pirThreshold_},
    gain	{gain_}
#else
    gain	{gain_},
    pirThreshold{pirThreshold_},
    actEnable	{actEnable_},
    retrigger	{retrigger_},
    pirEnable	{pirEnable_},
    lvdEnable	{lvdEnable_},
    lvd		{lvd_}
#endif
{}

HT7M2xxxMotionSensor::Configuration1
HT7M2xxxMotionSensor::getConfiguration1() const {
    uint16_t value = getRegister(RegisterKey::configuration1);
    return *reinterpret_cast<Configuration1 *>(&value);
}

void HT7M2xxxMotionSensor::setConfiguration1(Configuration1 value) const {
    setRegister(RegisterKey::configuration1, *reinterpret_cast<uint16_t *>(&value));
}

HT7M2xxxMotionSensor::Configuration2::Configuration2(
    uint8_t	address_,
    uint8_t	luminanceThreshold_)
:
#if BYTE_ORDER == BIG_ENDIAN
    luminanceThreshold	{luminanceThreshold_},
    address		{address_}
#else
    address		{address_},
    luminanceThreshold	{luminanceThreshold_}
#endif
{}

HT7M2xxxMotionSensor::Configuration2
HT7M2xxxMotionSensor::getConfiguration2() const {
    uint16_t value = getRegister(RegisterKey::configuration2);
    return *reinterpret_cast<Configuration2 *>(&value);
}

void HT7M2xxxMotionSensor::setConfiguration2(Configuration2 value) const {
    setRegister(RegisterKey::configuration2, *reinterpret_cast<uint16_t *>(&value));
}

uint16_t HT7M2xxxMotionSensor::getTriggerTimeInterval() const {
    return translate(getRegister(RegisterKey::triggerTimeInterval));
}

void HT7M2xxxMotionSensor::setTriggerTimeInterval(uint16_t value) const {
    setRegister(RegisterKey::triggerTimeInterval, translate(value));
}

uint16_t HT7M2xxxMotionSensor::getPirRawData() const {
    return translate(getRegister(RegisterKey::pirRawData));
}

uint16_t HT7M2xxxMotionSensor::getOpticalSensorRawData() const {
    return translate(getRegister(RegisterKey::opticalSensorRawData));
}

uint16_t HT7M2xxxMotionSensor::getTemperatureSensorRawData() const {
    return translate(getRegister(RegisterKey::temperatureSensorRawData));
}

HT7M2xxxMotionSensor::Status HT7M2xxxMotionSensor::getStatus() const {
    uint16_t value = getRegister(RegisterKey::status);
    return *reinterpret_cast<Status *>(&value);
}

uint16_t HT7M2xxxMotionSensor::getManufacturerId() const {
    return translate(getRegister(RegisterKey::manufacturerId));
}

uint16_t HT7M2xxxMotionSensor::getFirmwareVersion() const {
    return translate(getRegister(RegisterKey::firmwareVersion));
}

unsigned HT7M2xxxMotionSensor::tillAvailable() const {
    return 1000;
}

unsigned HT7M2xxxMotionSensor::increaseSensitivity() {
    return tillAvailable();
}

unsigned HT7M2xxxMotionSensor::decreaseSensitivity() {
    return tillAvailable();
}

float HT7M2xxxMotionSensor::readMotion() {
    uint16_t pirRawData = getPirRawData();
    uint16_t delta = pirRawData > this->pirRawData
	? pirRawData - this->pirRawData
	: this->pirRawData - pirRawData;
    ESP_LOGI(name, "raw %c %04x %04x %04x",
	0x100 < delta ? '*' : ' ',
	static_cast<unsigned>(delta),
	static_cast<unsigned>(pirRawData),
	static_cast<unsigned>(this->pirRawData));
    this->pirRawData = pirRawData;
    return delta;
}

HT7M2xxxMotionSensor::~HT7M2xxxMotionSensor() {}
