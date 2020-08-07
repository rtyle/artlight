#include <cinttypes>

#include <esp_log.h>

#include "HT7M2xxxMotionSensor.h"

// symmetric encode/decode for 16 bit value
// between BYTE_ORDER of our processor
// and BIG_ENDIAN encoding of device registers
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
    i2cMaster		{i2cMaster_}
{
    assertId();
    timer.start();
}

// how long we should wait for commands to complete
static TickType_t constexpr wait {1};

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
    assert(0x04d9 == getManufacturerId());
}

HT7M2xxxMotionSensor::Configuration0::Configuration0(
    signed	temperatureCelsius_,
    unsigned	pirPeriod_)
:
    temperatureCelsius	{temperatureCelsius_},
    pirPeriod		{pirPeriod_}
{}

HT7M2xxxMotionSensor::Configuration0
HT7M2xxxMotionSensor::getConfiguration0() const {
    uint16_t const value {getRegister(RegisterKey::configuration0)};
    return *reinterpret_cast<Configuration0 const *>(&value);
}

void HT7M2xxxMotionSensor::setConfiguration0(Configuration0 value) const {
    setRegister(RegisterKey::configuration0, *reinterpret_cast<uint16_t *>(&value));
}

HT7M2xxxMotionSensor::Configuration1::Configuration1(
    unsigned	lowVoltageDetectThreshold_,
    bool	lowVoltageDetectEnable_,
    bool	pirDetectEnable_,
    bool	pirRetrigger_,
    bool	pirTriggeredPinEnable_,
    unsigned	pirThreshold_,
    unsigned	pirGain_)
:
#if BYTE_ORDER == BIG_ENDIAN
    lowVoltageDetectThreshold	{lowVoltageDetectThreshold_},
    lowVoltageDetectEnable	{lowVoltageDetectEnable_},
    pirDetectEnable		{pirDetectEnable_},
    pirRetrigger		{pirRetrigger_},
    pirTriggeredPinEnable	{pirTriggeredPinEnable_},
    pirThreshold		{pirThreshold_},
    pirGain			{pirGain_}
#else
    pirTriggeredPinEnable	{pirTriggeredPinEnable_},
    pirRetrigger		{pirRetrigger_},
    pirDetectEnable		{pirDetectEnable_},
    lowVoltageDetectEnable	{lowVoltageDetectEnable_},
    lowVoltageDetectThreshold	{lowVoltageDetectThreshold_},
    pirGain			{pirGain_},
    pirThreshold		{pirThreshold_}
#endif
{}

HT7M2xxxMotionSensor::Configuration1 &
HT7M2xxxMotionSensor::Configuration1::setPirThreshold(unsigned value) {
    pirThreshold = value;
    return *this;
}

HT7M2xxxMotionSensor::Configuration1 &
HT7M2xxxMotionSensor::Configuration1::setPirGain(unsigned value) {
    pirGain = value;
    return *this;
}

HT7M2xxxMotionSensor::Configuration1
HT7M2xxxMotionSensor::getConfiguration1() const {
    uint16_t const value {getRegister(RegisterKey::configuration1)};
    return *reinterpret_cast<Configuration1 const *>(&value);
}

void HT7M2xxxMotionSensor::setConfiguration1(Configuration1 value) const {
    setRegister(RegisterKey::configuration1, *reinterpret_cast<uint16_t *>(&value));
}

HT7M2xxxMotionSensor::Configuration2::Configuration2(
    unsigned	darkThreshold_,
    bool	pirDetectIfDarkEnable_,
    unsigned	address_)
:
#if BYTE_ORDER == BIG_ENDIAN
    darkThreshold		{darkThreshold_},
    pirDetectIfDarkEnable	{pirDetectIfDarkEnable_},
    address			{address_}
#else
    pirDetectIfDarkEnable	{pirDetectIfDarkEnable_},
    darkThreshold		{darkThreshold_},
    address			{address_}
#endif
{}

HT7M2xxxMotionSensor::Configuration2
HT7M2xxxMotionSensor::getConfiguration2() const {
    uint16_t const value {getRegister(RegisterKey::configuration2)};
    return *reinterpret_cast<Configuration2 const *>(&value);
}

void HT7M2xxxMotionSensor::setConfiguration2(Configuration2 value) const {
    setRegister(RegisterKey::configuration2, *reinterpret_cast<uint16_t *>(&value));
}

unsigned HT7M2xxxMotionSensor::getCachedTriggerTimeInterval() const {
    return cachedTriggerTimeInterval;
}

unsigned HT7M2xxxMotionSensor::getTriggerTimeInterval() const {
    return translate(getRegister(RegisterKey::triggerTimeInterval));
}

void HT7M2xxxMotionSensor::setTriggerTimeInterval(unsigned value) {
    setRegister(RegisterKey::triggerTimeInterval, translate(value));
    cachedTriggerTimeInterval = value;
}

unsigned HT7M2xxxMotionSensor::getPirRawData() const {
    return translate(getRegister(RegisterKey::pirRawData));
}

unsigned HT7M2xxxMotionSensor::getOpticalSensorRawData() const {
    return translate(getRegister(RegisterKey::opticalSensorRawData));
}

unsigned HT7M2xxxMotionSensor::getTemperatureSensorRawData() const {
    return translate(getRegister(RegisterKey::temperatureSensorRawData));
}

HT7M2xxxMotionSensor::Status HT7M2xxxMotionSensor::getStatus() const {
    uint16_t const value {getRegister(RegisterKey::status)};
    return *reinterpret_cast<Status const *>(&value);
}

unsigned HT7M2xxxMotionSensor::getManufacturerId() const {
    return translate(getRegister(RegisterKey::manufacturerId));
}

unsigned HT7M2xxxMotionSensor::getFirmwareVersion() const {
    return translate(getRegister(RegisterKey::firmwareVersion));
}

unsigned HT7M2xxxMotionSensor::period() const {
    return 200;
}

bool HT7M2xxxMotionSensor::readMotion() {
#if 0
    {
	Configuration0 const c0 {getConfiguration0()};
	Configuration1 const c1 {getConfiguration1()};
	Configuration2 const c2 {getConfiguration2()};
	unsigned const triggerTimeInterval {getTriggerTimeInterval()};
	unsigned const pirRawData {getPirRawData()};
	ESP_LOGI(name, "\ttemperature %d°C\tpirPeriod %ums\tlowVoltageDetectThreshold %u\tlowVoltageDetectEnable %u\tpirDetectEnable %u\tpirRetrigger %u\tpirTriggeredPinEnable %u\tpirThreshold %.1fv\tpirGain %u\tdarkThreshold %u\tpirDetectIfDarkEnable %u\taddress %02x\ttriggerTimeInterval %ums\tpirRawData %u",
	    c0.temperatureCelsius,
	    pirPeriodDecode(c0.pirPeriod),
	    c1.lowVoltageDetectThreshold,
	    c1.lowVoltageDetectEnable,
	    c1.pirDetectEnable,
	    c1.pirRetrigger,
	    c1.pirTriggeredPinEnable,
	    pirThresholdDecode(c1.pirThreshold),
	    pirGainDecode(c1.pirGain),
	    c2.darkThreshold,
	    c2.pirDetectIfDarkEnable,
	    c2.address,
	    triggerTimeInterval * 100,
	    pirRawData);
    }
#endif
    Status const status {getStatus()};
#if 0
    ESP_LOGI(name, "%c\tnotReady %u\tlowVoltageDetected %u\tdark %u\tpirNoiseDetected %u\tpirRetriggered %u\tpirTriggered %u",
	status.pirTriggered ? '*' : ' ',
	status.notReady,
	status.lowVoltageDetected,
	status.dark,
	status.pirNoiseDetected,
	status.pirRetriggered,
	status.pirTriggered);
#endif
    return status.pirTriggered;
}

HT7M2xxxMotionSensor::~HT7M2xxxMotionSensor() {}
