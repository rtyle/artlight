Components on custom artlight board

	Adafruit HUZZAH32 – ESP32 Feather Board

		https://www.adafruit.com/product/3405

	Adafruit TSL2561 Digital Luminosity/Lux/Light Sensor Breakout

		https://www.adafruit.com/product/439

	Adafruit Breadboard-friendly 2.1mm DC barrel jack

		https://www.adafruit.com/product/373
	
	Two 1K 1/4 watt resistors

	2 pin, 2.54mm pitch, 90 degree header

	4 pin, 2.54mm pitch, 90 degree header

./eagle/projects/artlight contains files used to create an artlight PCB

	Adafruit eagle files from github

		Adafruit HUZZAH32 ESP32 Feather.brd
		Adafruit HUZZAH32 ESP32 Feather.sch

		TSL2561_5V_REV-A.brd
		TSL2561_5V_REV-A.sch

	Eagle project file, board, schematic and generated CAM data sent to JLCPCB

		eagle.epf
		artlight.brd
		artlight.sch
		artlight.zip

./eagle/libraries contains supporting libraries

	artlight.lbr

		Eagle Devices for Huzzah32 and TSL2561, as used by artlight
		
	PJ-102A.lbr

		Eagle Device for barrel jack

Board assembly

	Solder resistors.

	Solder 90 degree pin headers.

	Solder the TSL2561 and Huzzah32 to the PCB using pin headers.
	Only pins that are used can be connected.

External components

	Adafruit Dotstar devices ...

		https://www.adafruit.com/category/885

	... for example, LED strips

		https://www.adafruit.com/product/2241

	These may be controlled by the HSPI and VSPI signals on the four pin header.
	SCLK signals are on the outside, MOSI on the inside.
	Facing the board, top side up, HSPI is on the right, VSPI on the left.
	3.3V signals seem to work OK without a 3.3V to 5V level shifter.
	
	A 5.5mm x 2.1mm Y Splitter Cable should be used to power both the board and the lights.
	For example,

		https://www.adafruit.com/product/1351

	Enough power should be supplied for both.
	For example,

		https://www.adafruit.com/product/1466

	A jumper across the 2 pin 5V-USB header should be applied
	to connect the 5V supply to the USB pin of the Huzzah32.
	This jumper should be removed when using a USB cable to flash or monitor the ESP32.
	The 5V supply should always be connected
	so that the lights and board share the same ground reference.
