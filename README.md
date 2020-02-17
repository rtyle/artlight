# artlight
This repository supports similar microprocessor-controlled applications
that artfully control individually addressable LEDs.

Currently, there is a corhole and a clock application for this repository.

## Parts

Common parts are
* [ESP32 board](https://www.adafruit.com/product/3405)
* [TSL2561 digital light sensor breakout](https://www.adafruit.com/product/439)
* [DotStar digital LED strips](https://www.adafruit.com/product/2241)
* [2.1mm DC barrel jack](https://www.adafruit.com/product/373)

### Clock parts

Additional parts for the clock application are
* [40 inch metal wall art](https://www.etsy.com/listing/614388701/round-metal-wall-art-perfect)
* [Custom PCB board for microprocessor](https://github.com/rtyle/artlight/blob/master/eagle/projects/artlight/artlight.brd)
* [Custom PCB board for 12 o'clock](https://github.com/rtyle/artlight/blob/master/eagle/projects/ray/ray0.brd)
* [Custom PCB board for other o'clock](https://github.com/rtyle/artlight/blob/master/eagle/projects/ray/ray.brd)
* [Custom PCB board](https://github.com/rtyle/artlight/blob/master/eagle/projects/artlight/artlight.brd)
* [Mounting tape](https://www.amazon.com/gp/product/B00PKI7IBG)
* [Power supply](https://www.adafruit.com/product/1466)
* [Power Y splitter](https://www.amazon.com/gp/product/B06Y5GP7SF)

### Cornhole parts

Additional parts for the cornhole application are
* Cornhole board
* [Custom PCB board](https://github.com/rtyle/artlight/blob/master/eagle/projects/artlight/artlight.cornhole.brd)
* [Corhole LED ring](https://www.amazon.com/gp/product/B01N7SRCQJ)
* [IR break beam sensor](https://www.adafruit.com/product/2168)
* [Vibration sensor](https://www.adafruit.com/product/1766)
* [On/off RGB switch](https://www.adafruit.com/product/3424)
* [Momentary RGB switch](https://www.adafruit.com/product/3423)
* [NPN bipolar transistors](https://www.adafruit.com/product/756)
* [Resistors](https://www.adafruit.com/product/2784)

## Construction

The board components were soldered on their respective boards. When powered by USB, the 5V power jumper must first be removed.

### Clock

The microprocessor board is mounted on the back of the art with its luminosity sensor peeking through a hole drilled in the center. Power is routed and split between the microprocessor and 12 o'clock boards. SPI signals used to animate the individually addressable LEDs are cabled from the microprocessor board to the 12 o'clock board.

LED strips are mounted to the back of the wall art. To reduce wiring complexity, the 12 rays are supported by PCBs. 12 o'clock is special as that is where power and control signals are introduced so there is a special board for it. Each ray is cut to length, an LED strip is cut to match, mounted with tape and soldered at the ends. The ray boards are then mounted to the back of the art using more of the same tape. Small strips were cut to bridge the arcs between ray bases and soldered in place. Starting at 12 o'clock, strips were cut and joined together with wire to skirt the perimeter of the art. Only clock and data signals need to jump over the rays as 5V and GND can be tapped from each ray.

### Cornhole
