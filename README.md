# artlight
This repository supports similar microprocessor-controlled applications
that artfully animate individually addressable LEDs.

Currently, there is a cornhole and a clock application supported by this repository.

## Parts

Common parts are
* [ESP32 board](https://www.adafruit.com/product/3405)
* [TSL2561 digital luminosity sensor breakout](https://www.adafruit.com/product/439)
* [DotStar digital LED strips](https://www.adafruit.com/product/2241)
* [2.1mm DC barrel jack](https://www.adafruit.com/product/373)

### Clock Specific Parts

Additional parts for the clock application are
* [40 inch metal wall art](https://www.etsy.com/listing/614388701/round-metal-wall-art-perfect)
* [Custom PCB board for microprocessor](https://github.com/rtyle/artlight/blob/master/eagle/projects/artlight/artlight.brd)
* [Custom PCB board for 12 o'clock ray](https://github.com/rtyle/artlight/blob/master/eagle/projects/ray/ray0.brd)
* [Custom PCB board for other rays](https://github.com/rtyle/artlight/blob/master/eagle/projects/ray/ray.brd)
* [Mounting tape](https://www.amazon.com/gp/product/B00PKI7IBG)
* [Power supply](https://www.adafruit.com/product/1466)
* [Power Y splitter](https://www.amazon.com/gp/product/B06Y5GP7SF)

### Cornhole Specific Parts

Additional parts for the cornhole application are
* Cornhole board
* [Custom PCB board](https://github.com/rtyle/artlight/blob/master/eagle/projects/artlight/artlight.cornhole.brd)
* [Cornhole LED ring](https://www.amazon.com/gp/product/B01N7SRCQJ)
* [IR break beam sensor](https://www.adafruit.com/product/2168)
* [Vibration sensor](https://www.adafruit.com/product/1766)
* [On/off RGB switch](https://www.adafruit.com/product/3424)
* [Momentary RGB switch](https://www.adafruit.com/product/3423)
* [NPN bipolar transistors](https://www.adafruit.com/product/756)
* [Resistors](https://www.adafruit.com/product/2784)
* [Power bank](https://www.amazon.com/gp/product/B072MH1434)
* [Magnetic mount](https://www.amazon.com/gp/product/B01M0DUVZV)

## Construction

The PCB components were soldered on their respective microprocessor boards.
When powered by USB, the 5V power jumper must first be removed.

### Clock

The microprocessor board is mounted on the back of the art with its luminosity sensor peeking through a hole drilled in the center.
Power is routed and split between the microprocessor and 12 o'clock ray boards.
SPI signals used to animate the individually addressable LEDs are cabled from the microprocessor board to the 12 o'clock board.

LED strips are mounted to the back of the wall art.
To reduce wiring complexity, the 12 rays are supported by PCBs.
12 o'clock is different so there is a special board for it.
Each ray is cut to length, an LED strip is cut to match, mounted with tape and soldered at the ends with LEDs advancing toward the center.
The ray boards are then mounted to the back of the art using more of the same tape.
Small strips are cut to bridge the arcs between ray bases, taped and soldered in place with LEDs advancing clockwise.
Starting at 12 o'clock, strips are cut and joined together with wire to skirt the perimeter of the art (clockwise).
Only clock and data signals need to tunnel under the rays as 5V and GND can be tapped from the LED strip edges of adjacent rays.

### Cornhole

The power bank is mounted with strong magnets under the board so that it might easily be unplugged, removed, charged, replaced and plugged back in.
Power from it may be switched on and off (with the on/off switch).
Switched power is split between the microprocessor board, the LED ring strip and the RGB LEDs in all the switches.

The R, G and B LEDs in the switches are separtely pulse-width-modulated (PWM) to mix the preferred color of each.
PWM switching is controlled indirectly through a transistor circuits wired to R, G and B of each switch.
The momentary switch outputs are wired to the microprocessor board inputs dedicated for them.

Vibration sensors are strategically placed (for best performance) under the board.
They are wired in parallel to the microprocessor board input dedicated for them.

IR break beam sensors are mounted across the cornhole ring for the best coverage (2 orthogonal pairs seems adequate).
They are wired in parallel to the microprocessor board input dedicated for them.

The LEDs that come with the cornhole LED ring are replaced with 80 individually addressable LEDs in a clockwise (from the top) strip.
Their control lines are wired to the microprocessor board output dedicated for them.
The ring is mounted under the board.
For best results, the material above the LED ring should be transparent (for example, an acrylic ring)
so that the lights under the board can be seen from above.

## Use

Both applications must be provisioned on a Wi-Fi LAN.
Until this is done, they will offer their own Wi-Fi access point (at 192.168.4.1) where they can be accessed by a web browser (https://192.168.4.1) to perform this provisioning.
The SSID and Password for Wi-Fi access must be entered.
Optionally, a mDNS hostname may be given.

Once provisioned, preferences may be set for each application by pointing a web browser to the device.
If supported, the mDNS hostname may be used to resolve the device address;
otherwise, the leased address will have to be resolved according to the capabilities of the LAN's DHCP server.
If possible, name the device and configure DNS services on the LAN to resolve the name to this address.

Preferences may be used to customize the presentation (color, width and shape) of LED indicators.
Brightness parameters may be set which may include use of the luminosity sensor.
Parameters may be set in order to accurately present time in the given time zone.
Over-the-air updates may also be performed.

## Clock Use

There are no additional preferences for the clock application.

The clock will present time by animating hour, minute and second indicators using the LEDs on the back of the art.
Hour and minute indicators will animate along the up and down the rays and inner arcs that connect them.
The second indicator will sweep across the perimeter.

## Cornhole Use


