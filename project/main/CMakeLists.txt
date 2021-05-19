# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS
	APA102.cpp
	AsioTask.cpp
	Button.cpp
	Contrast.cpp
	Curve.cpp
	DialPreferences.cpp
	Event.cpp
	fromString.cpp
	GammaEncode.cpp
	HT7M2xxxMotionSensor.cpp
	Httpd.cpp
	I2C.cpp
	InRing.cpp
	KeyValueBroker.cpp
	LEDC.cpp
	LightPreferences.cpp
	LuxSensor.cpp
	main.cpp
	MDNS.cpp
	MotionSensor.cpp
	NVSKeyValueBroker.cpp
	OtaTask.cpp
	PCA9685.cpp
	PeerTask.cpp
	percentDecode.cpp
	Pin.cpp
	Preferences.cpp
	ProvisionTask.cpp
	Pulse.cpp
	Qio.cpp
	SensorTask.cpp
	SmoothTime.cpp
	SPI.cpp
	Task.cpp
	TimePreferences.cpp
	Timer.cpp
	TimeUpdate.cpp
	TLC59711.cpp
	TSL2561LuxSensor.cpp
	TSL2591LuxSensor.cpp
	WebSocketTask.cpp
	Wifi.cpp
)
set(COMPONENT_ADD_INCLUDEDIRS "")

if(NOT DEFINED ENV{ArtlightApplication})
	message(FATAL_ERROR "ArtlightApplication not defined")
endif()
if(clock	STREQUAL $ENV{ArtlightApplication})
	set(COMPONENT_SRCS ${COMPONENT_SRCS} ClockArtTask.cpp)
	set_property(SOURCE ClockArtTask.cpp PROPERTY INCLUDE_DIRECTORIES $ENV{IDF_PATH}/components/newlib/priv_include)
	set_property(SOURCE main.cpp PROPERTY COMPILE_DEFINITIONS ArtLightApplication_h="ArtLightClock.h")
elseif(cornhole	STREQUAL $ENV{ArtlightApplication})
	set(COMPONENT_SRCS ${COMPONENT_SRCS} CornholeArtTask.cpp)
	set_property(SOURCE CornholeArtTask.cpp PROPERTY INCLUDE_DIRECTORIES $ENV{IDF_PATH}/components/newlib/priv_include)
	set_property(SOURCE main.cpp PROPERTY COMPILE_DEFINITIONS ArtLightApplication_h="ArtLightCornhole.h")
elseif(golden	STREQUAL $ENV{ArtlightApplication})
	set(COMPONENT_SRCS ${COMPONENT_SRCS} GoldenArtTask.cpp)
	set_property(SOURCE GoldenArtTask.cpp PROPERTY INCLUDE_DIRECTORIES $ENV{IDF_PATH}/components/newlib/priv_include)
	set_property(SOURCE main.cpp PROPERTY COMPILE_DEFINITIONS ArtLightApplication_h="ArtLightGolden.h")
elseif(nixie	STREQUAL $ENV{ArtlightApplication})
	set(COMPONENT_SRCS ${COMPONENT_SRCS} NixieArtTask.cpp)
	set_property(SOURCE NixieArtTask.cpp PROPERTY INCLUDE_DIRECTORIES $ENV{IDF_PATH}/components/newlib/priv_include)
	set_property(SOURCE main.cpp PROPERTY COMPILE_DEFINITIONS ArtLightApplication_h="ArtLightNixie.h")
else()
	message(FATAL_ERROR "ArtlightApplication $ENV{ArtlightApplication} unexpected")
endif()

set(COMPONENT_EMBED_FILES
	preferencesFavicon.ico
	provisionResponseFavicon
	../certificates/provision_ca_cert.pem
	../certificates/provision_ca_key.pem
)
if(NOT CMAKE_BUILD_EARLY_EXPANSION)
	add_custom_command(
		OUTPUT	preferences.html
		DEPENDS	${CMAKE_CURRENT_SOURCE_DIR}/preferences.html.m4
		COMMAND	m4 -DArtLightApplication=$ENV{ArtlightApplication} < ${CMAKE_CURRENT_SOURCE_DIR}/preferences.html.m4 > preferences.html
		VERBATIM
	)
endif()
set(COMPONENT_EMBED_TXTFILES
	${CMAKE_CURRENT_BINARY_DIR}/preferences.html
	../certificates/ota_ca_cert.pem
)

register_component()
