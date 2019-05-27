# digibaro2

Digital Barometer v2 (with e-Paper display).

Status:
- all supporting custom libraries are now written and include tests
- main application *not* developed yet
- schematic ready with preliminary board design
- 3D enclosure concept (wood enclosure for next iteration)

## Goal

Develop an open source digital barograph that does not require an external power
source, has a very low power consumption, and works on a boat independantly of
any wifi/cell connection.

## Motivation

I still believe that the barometric pressure is an essential decision making
tool when sailing. Displaying a graph rather than writing down single values
provides an instant evaluation of the trend.

The main idea is that the beautifull conventional mechanical barograph are not
suited for marine use (not resistant to vibration) and require a weekly paper
change.

![Classical Barograph](https://upload.wikimedia.org/wikipedia/commons/a/a0/Barograph_-_Image_from_page_45_of_%22Practical_physics%22_%281922%29.jpg)

The paper-electronic barographs aleviate this problem, but I remember that ours
was constantly jamming.

Several electronic barograph already exist on the market, but they all require
an external source of power (and are pricey).

## Rationale

Today you can have an app that will siphon the data from a nearby weather
station and display the barometric pressure in any format using your brillant
smartphone screen. And this is even not really necessary since having data
connectivity would allow to also get a full detailed weather forecast, much more
accurate, and covering a wider area, than what you could analyze yourself.

Despite our ever more connected world, I am old fashioned and like to believe
that having systems not relying on data flowing wirelessly to your boat can be
useful. It turns out that the fundamental building block for such a project is
readily available: several extremely accurate digital pressure sensors can be
sourced at low price (you may already have one in your cellphone).

I also wanted not to rely on an external source of power. This is not so much to
avoid battery drain (the most basic solar panel would provide more than enough),
but to avoid having extra wiring.

The final concept is that you would be able to leave the barograph on your boat,
and when coming back after six months, you would be able to read the barometric
pressure of the last few days because the barograph would keep collecting data.
Of course, this is probably superfluous since after leaving your boat six
months, you probably have more than 5 days of work before taking the sea again
(hence you could turn on the barograph).

At any rate, being a style exercise or a real need, the goal is to develop an
extremly low power barograph that could sustain at least six months on a set of
typical AA alkaline batteries!

There are few major difference between this project an my previous (never
completed) digibaro v1:

| Digibaro v1                 | Digibaro v2                   |
|-----------------------------|-------------------------------|
| Target low cost             | Cost is not the main priority |
| Classic UI with 5 buttons   | Mimimalist UI (analog feel)   |
| LCD screen                  | e-Paper display               |
| Only stores 5 days of data  | Long term recording (>20years)|
| Typical "maker" approach    | High reliability desired      |


## Hardware selection

The [first version of my digibaro](https://github.com/flupes/digibaro/) was
based one BMP085 sensor, a momochrome graphic LCD and an Arduino Pro-Mini
(3.3V). This version use modernized components.

The micro-controller is a ARM Cortex M0+ (SAMD21) packaged by RocketScream. This
board was selected because it was specifically designed for low power
applications (no extra LEDs, efficient regulator, etc.), and it provides 2MB of
flash memory on board.

The pressure sensor is a BOSH BME280. It provides excellent accuracy plus a
humidity and temperature sensor on the chip. The BMP680 would provide an extra
Volatile Organic gas detector, but a the cost of an extra complexity in managing
the sensor.

The display is based on a 400x300 black and white e-Paper system packaged by
Waveshare. E-paper display would seem ideal for this type of low power
application. However, since the refresh rate is so slow (~4s for this particular
display), updating the display takes a significant amount energy. There is a
cutting-point between a LCD (that refresh very quickly and consume very low
power in standby) and an e-Paper display that depends on the number of refresh
per day. Still, for this application, the e-Paper provides a nice, more natural,
look.

## Reference products

[Bohlken BG1512](http://www.bohlken.net/bg/bg1512_en.htm)
- Require external power source
- 10mA without backlight
- Retired
- ~300 Euro

[Aquatech DBX2](https://www.digitalbarograph.com/)
- Require external power source
- 20mA @ 5V DC (160mA with LED backlight)
- Storm alarm
- Factory calibrated
- [DBX2 User Manual](https://www.digitalbarograph.com/pdfs/DBX2_MANUAL_V1W.pdf)  
- ~$500

[NASA Meteoman](https://www.nasamarine.com/product/meteoman/)
- Consumes 100mA (without backlight), require external source of power
- Pressure change graph
- [Meteoman screenshots and discussion](http://www.ybw.com/forums/showthread.php?452897-Calling-Weather-Experts-What-do-you-Need-in-a-Barograph)
- ~$200

[Raber Barograf](http://www.barograph.ch/Barograph_E/barograph.html)
- Paper recording :-)
- Swiss made ;-)
- One single C type battery for a year!
- Very cool, however we had a similar model that jammed quite often.  
- [Meteograf User Manual](http://www.barograph.ch/Downloads/BA_Meteograf_Englisch_2.4.pdf)  
- ~$900 without case

## Related projects

In May 2019, I just discovered this: [Arduino + BMP085 Barograph from PauloDevelo on Arbatus](https://www.arbutus.ecogium.fr/the-boat/electronic-equipment/barograph-arbutus/).

<!-- 
Code at: https://github.com/PauloDevelo/Barograph/blob/master/src/barograph.ino

This is similar to to what I started to develop with v1 of digibaro. However,
PauloDevelo chose a much simpler route: it is again powered from the boat
batteries (so not need for sleep mode), there is no RTC, no real concept of time
series, no user interface per say. This allowed him to complete the project
(unlike my digibaro v1). Congratulations! In addition, he created a nice wooden
enclosure.
-->
