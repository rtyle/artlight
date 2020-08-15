#include "LightPreferences.h"

char const * const LightPreferences::Dim::string[] {"automatic", "manual",};
LightPreferences::Dim::Dim(Value value_) : value(value_) {}
LightPreferences::Dim::Dim(char const * value) : value(
    [value](){
	size_t i = 0;
	for (auto e: string) {
	    if (0 == std::strcmp(e, value)) return static_cast<Value>(i);
	    ++i;
	}
	return automatic;
    }()
) {}
char const * LightPreferences::Dim::toString() const {
    return string[value];
}

char const * const LightPreferences::Range::string[] {"clip", "normalize",};
LightPreferences::Range::Range(Value value_) : value(value_) {}
LightPreferences::Range::Range(char const * value) : value(
    [value](){
	size_t i = 0;
	for (auto e: string) {
	    if (0 == std::strcmp(e, value)) return static_cast<Value>(i);
	    ++i;
	}
	return clip;
    }()
) {}
char const * LightPreferences::Range::toString() const {
    return string[value];
}

LightPreferences::LightPreferences(
    asio::io_context &		io,
    KeyValueBroker &		keyValueBroker)
:
    range(Range::clip),
    rangeObserver(keyValueBroker, "range", range.toString(),
	[this, &io](char const * value){
	    Range range_(value);
	    io.post([this, range_](){
		range = range_;
	    });
	}),
    dim(Dim::automatic),
    dimObserver(keyValueBroker, "dim", dim.toString(),
	[this, &io](char const * value){
	    Dim dim_ (value);
	    io.post([this, dim_](){
		dim = dim_;
	    });
	}),
    dimLevel(16),
    dimLevelObserver(keyValueBroker, "dimLevel", "16",
	[this, &io](char const * value){
	    unsigned const level = std::strtoul(value, nullptr, 10);
		if (3 <= level && level <= 16) {
		io.post([this, level](){
		    dimLevel = level;
		});
	    }
	}),
    gammaEncode(2.0),
    gamma(20),
    gammaObserver(keyValueBroker, "gamma", "20",
	[this, &io](char const * value){
	    unsigned const level = std::strtoul(value, nullptr, 10);
	    if (5 <= level && level <= 30) {
		io.post([this, level](){
		    gamma = level;
		    gammaEncode.gamma(gamma / 10.0f);
		});
	    }
	})
{}
