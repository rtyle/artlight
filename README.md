# artlight
This repository supports similar applications that artfully animate light.

Currently, there is a cornhole, wall clock and nixie tube clock application supported by this repository.

Nixie tube clock: https://youtu.be/5ZdNDDC60QM

Wall clock: https://youtu.be/0ISKXnE9frU

## Parts

### Nixie Tube Clock Parts

* [Bill of Materials](https://github.com/rtyle/artlight/blob/master/easyeda/projects/nixie/bom.csv), plus...
* [AC to 12V DC 1A wall mount power adapter](https://www.digikey.com/product-detail/en/cui-inc/SWI10-12-N-P5/102-4138-ND/6618698)
* 2 [Harwin 15 pin 2.54mm pitch](https://www.mouser.com/ProductDetail/855-M20-8771546) for SMT mount of Huzzah32 board
* 44 [Harwin H3161-05](https://www.digikey.com/product-detail/en/harwin-inc/H3161-05/952-2204-ND/3728169) sockets for nixie tube pins

Optional off-board ambient light sensor for automatic dimming
* [TSL2591 high dynamic range digital light sensor](https://www.adafruit.com/product/1980)
* [SparkFun Qwiic adapter](https://www.sparkfun.com/products/14495) TSL2591 qwiic mount
* 4 [Jumper SSH-003T-P0.2-H X2 12"](https://www.digikey.com/product-detail/en/jst-sales-america-inc/ASSHSSH28K305/455-3077-ND/6009453) TSL2591 qwiic cable
* 2 [Connector housing SH 4pos 1mm white](https://www.digikey.com/product-detail/en/jst-sales-america-inc/SHR-04V-S/455-1394-ND/759883) TSL2591 qwiic connectors

Optional on-board motion sensor plugin module for automatic on/off control
* [PIR detector I2C module](https://www.holtek.com/productdetail/-/vg/7M21x6)
* [Preci-Dip 851-87-006-10-001101 PCB connector](https://www.digikey.com/product-detail/en/preci-dip/851-87-006-10-001101/1212-1350-ND/3757600) PIR I2C mount

Optional base (something is required)
* 1 foot [Cast acrylic tubing 2.000" OD x .250" wall thickness](), cut into 9/16" slices. Four used as pedestals under PCB's nixie tubes.

### Cornhole and Wall Clock Parts

#### Cornhole and Wall Clock Common Parts

* [ESP32 board](https://www.adafruit.com/product/3405)
* [TSL2561 digital luminosity sensor breakout](https://www.adafruit.com/product/439)
* [DotStar digital LED strips](https://www.adafruit.com/product/2241)
* [2.1mm DC barrel jack (through hole)](https://www.adafruit.com/product/373)
* [Wi-Fi range extender (optional)](https://www.amazon.com/gp/product/B072MH1434)

#### Wall Clock Specific Parts

Additional parts for the clock application are
* [40 inch metal wall art](https://www.etsy.com/listing/614388701/round-metal-wall-art-perfect)
* [Custom PCB board for microprocessor](https://github.com/rtyle/artlight/blob/master/eagle/projects/artlight/artlight.brd)
* [Custom PCB board for 12 o'clock ray](https://github.com/rtyle/artlight/blob/master/eagle/projects/ray/ray0.brd)
* [Custom PCB board for other rays](https://github.com/rtyle/artlight/blob/master/eagle/projects/ray/ray.brd)
* [Mounting tape](https://www.amazon.com/gp/product/B00PKI7IBG)
* [Power supply](https://www.adafruit.com/product/1466)
* [Power Y splitter](https://www.amazon.com/gp/product/B06Y5GP7SF)
* [2.1mm DC barrel jack (surface mount)](https://www.sparkfun.com/products/12748)

#### Cornhole Specific Parts

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

The PCB components were soldered on their respective boards.
When Huzzah32 is powered by USB cable, the 5V power jumper must first be removed.

### Nixie Tube Clock Construction

[PCB design files](https://github.com/rtyle/artlight/tree/master/easyeda/projects/nixie)

For structural integrity, use a thick (>= 2.0mm) PCB.
Order PCB with a solder paste stencil.
Use the stencil to distribute solder paste for all components under the nixie tubes and solder them with a hot air gun.
Solder all other components individually, as appropriate.
Press in pin sockets and solder from the bottom.
Mount nixie tubes and place PCB on acrylic pedestals.

*WARNING*: The nixie tubes run at 170 volts.
To reduce exposure, do not plug in until tubes are mounted and PCB is placed on acrylic pedestals.

### Wall Clock Construction

[PCB design (artlight.\*) files](https://github.com/rtyle/artlight/tree/master/eagle/projects/artlight)
with
[boards for mounting LED strips on rays](https://github.com/rtyle/artlight/tree/master/eagle/projects/ray)

The microprocessor board is mounted on the back of the art with its luminosity sensor peeking through a hole drilled in the center.
Power is routed and split between the microprocessor and 12 o'clock ray boards.
SPI signals used to animate the individually addressable LEDs are cabled from the microprocessor board to the 12 o'clock board.

LED strips are mounted to the back of the wall art.
To reduce wiring complexity, the 12 rays are supported by PCBs.
12 o'clock is different so there is a special board for it.
Each ray is cut to length, an LED strip is cut to match, mounted with tape and soldered at the ends with LEDs advancing toward the center.
The ray boards are then mounted to the back of the art using more of the same tape.
Small strips are cut to bridge the arcs between ray bases, taped and soldered in place with LEDs advancing clockwise (from front!).
Starting at 12 o'clock, strips are cut and joined together with wire to skirt the perimeter of the art (clockwise, from front).
Only clock and data signals need to tunnel under the rays as 5V and GND can be tapped from the LED strip edges of adjacent rays.

### Cornhole Construction

[PCB design (artlight.cornhole.\*) files](https://github.com/rtyle/artlight/tree/master/eagle/projects/artlight)

The power bank is mounted with strong magnets under the board so that it might easily be unplugged, removed, charged, replaced and plugged back in.
Power from it may be switched on and off (with the on/off switch).
Switched power is split between the microprocessor board, the ring LED strip and the high side of the RGB LEDs in all the switches.

The R, G and B LEDs in the switches are separtely pulse-width-modulated (PWM) to mix the preferred color of each.
PWM switching is controlled indirectly through transistor circuits wired to the low side of the RGB LEDs of each switch.
The momentary switch outputs (closed to ground) are wired to the microprocessor board inputs dedicated for them.

Vibration sensors are strategically placed (for best performance) under the board.
The vibration sensor part may be changed to affect its sensitivity.
Their shared (parallel) output (closed to ground) is wired to the microprocessor board input dedicated for them.

IR break beam sensors are mounted across the cornhole ring for the best coverage (2 orthogonal pairs seems adequate).
Their shared (parallel) output (closed to ground) is wired to the microprocessor board input dedicated for them.

The LEDs that come with the cornhole LED ring are replaced with a clockwise (from the top) strip of 80 individually addressable LEDs.
Their control lines are wired to the microprocessor board output dedicated for them.
The ring is mounted under the board.
For best results, the material above the LED ring should be transparent (for example, an acrylic ring)
so that the lights under the board can be seen from above.

## Use

Applications must be provisioned on a Wi-Fi LAN.
Until this is done, they will offer their own Wi-Fi access point where they can be configured securely by a web browser (https://192.168.4.1).
The SSID and password for Wi-Fi access must be entered.
Optionally, an mDNS hostname may be given.

The reach of your Wi-Fi LAN may need to be increased to reach these devices.
This is best done by repositioning the devices and/or Wi-Fi access points.
If necessary, deploy a Wi-Fi range extender.

Once provisioned, preferences may be set for each application by pointing a web browser to the device (http://\<device\>/).
If supported, the mDNS hostname may be used to resolve the device address;
otherwise, the leased IP address will have to be resolved (perhaps manually) depending on the capabilities of the LAN's DHCP/DNS services.

The preferences page may be filled using "factory" default values or with the current values.
Changes to most will take effect immediately.
If not, then they will take effect when the form is submitted.

Preferences may be used to customize the presentation.
Brightness parameters may be set which may include use of the luminosity sensor.
The mDNS hostname of the board may be changed.
Parameters may be set in order to accurately present time in the given time zone.
Over-the-air software updates may also be performed.

## Nixie Tube Clock Use

The bottom dot of the colon pulses at 30 beats per minute (a second on and a second off).
The top dot of the colon pulses at 31 beats per minute.
This results in pulsing dots that are in phase at the beginning of the minute.
They separate more as mid-minute is approached and come back together again as the next minute is approached.
Thus the colon is a very crude seconds display.

Dimming controls should be slid all the way to the left to disable dimming; otherwise, dimming will start to occur as it gets dark.
The further to the right the dimming control is set, the more dimming will be done.
When slid all the way to the right, the controlled element will dim to black when the room is black.

In an effort to combat [nixie tube cathode poisoning](https://www.google.com/search?q=nixie+tube+cathode+poisoning)
the clock will not display minutes during the first minute of every hour.
Instead, for the first half of the minute, the normal minutes-places will cycle through digits that are not normally used there.
In the second half of the minute, the hour will be displayed in the normal minutes-places
and the normal hours-places will cycle through digits that are not normally used there.
The display can also be temporarily put in a Clean mode (or other modes) until expectations are met.

## Wall Clock Use

There are no additional preferences for the clock application.

The clock will present time by animating hour, minute and second indicators using the LEDs mounted to the back of the art.
Hour and minute indicators will animate up and down the rays and across the inner arcs that connect them.
The second indicator will sweep across the perimeter.

## Cornhole Use

The cornhole "preferences" page may be used to keep score.
In fact, unless explicitly accessed through a URL with a preferences path (http://\<device\>/preferences), scoring is the only thing allowed.

Changing most preferences (including scoring) will be shared between boards that have been configured to use the same network port number.
For this to work effectively, only the two boards used in a cornhole game should share the same port number and each must be reachable from the other on the same network.

A preference may be used to control what the ring LED strip indicates:
* The score of each team
* The time
* Various animations

Regardless of what the ring indicates:

A short press of a momentary switch will increment the score of the associated team.
A long press of a momentary switch will decrement the score of the associated team while held.
While decrementing, if the other switch is down too then the score is reset to 0 (hold both to reset both to 0).

Vibration from the board will cause a short "lightning strike" animation to be presented over the existing ring indication.
A break in the ring's IR beam sensors will cause a short "fireworks" animation to be presented over the existing ring indication.

## Software Build, Deploy and Monitor

These projects were built on a Fedora Linux platform.
Your mileage may vary elsewhere.

Get toolchain

    sudo dnf install gcc git wget make ncurses-devel flex bison gperf python pyserial future python2-cryptography pyparsing
    mkdir -p $HOME/esp
    (
      cd $HOME/esp
      wget https\://dl.espressif.com/dl/xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz
      tar xzf xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz
    )
    
Get artlight source code with it's dependencies

    git clone http://github.com/rtyle/artlight.git
    cd artlight
    git submodule update --init --recursive
  
[Create certificates](https://github.com/rtyle/artlight/blob/master/README.certificates.txt)

artlightMake shortcut

    artlightMake() { PATH=$PATH:$HOME/esp/xtensa-esp32-elf/bin IDF_PATH=esp-idf make ArtLightApplication=$application $* ; }
    artlightMake help

Select application (choose one)

    application=clock
    application=cornhole
    application=nixie

Build application

    artlightMake clean
    artlightMake all

Grant user access to USB serial devices then **relogin** to get access

    sudo usermod -a -G dialout $USER

Prepare /dev/ttyUSB0 (default) connected device

     artlightMake erase_flash
     artlightMake erase_otadata

Deploy application through /dev/ttyUSB0 (default) connected device

    artlightMake flash

Monitor device

    artlightMake monitor
