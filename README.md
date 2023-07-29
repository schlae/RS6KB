# RS6KB - Use PS/2 Keyboards with RS/6000s
Old RS/6000 computers don't fully support PS/2 keyboards. They were originally designed to use a Model M keyboard with lightly modified firmware and a speaker. (Ever wonder what the speaker grille is for inside such a keyboard?)

The firmware change implements a single new command, 0xEF. This queries the keyboard to find out if it uses a standard 101 key layout or the "world trade" 102 key layout. Unfortunately, if the computer doesn't receive a response to this command, it generates a keyboard error and won't boot into service mode correctly. The firmware in this project is hard coded to the US 101 key layout but by changing the response byte 0xB0 to 0xB1 it can be made to work with the 102 key "world trade" (ISO) layout.

| Host | Keyboard | Keyboard | Keyboard |
| ---- | -------- | -------- | -------- |
| 0xEF (Layout ID) | 0xFA (Ack) | 0xB0 (US 101 key) or 0xB1 (WT 102 key) | 0xBF |

For technical details of the protocol, see page 356 of the [RS/6000 Hardware Technical Reference General Information](http://bitsavers.org/pdf/ibm/rs6000/SA23-2643-00_RS6000_Hardware_Technical_Reference_General_Information_1990.pdf) manual.

This project corrects the issue by snooping the PS/2 bus. It watches for the 0xEF command, and when that comes through, it disconnects the keyboard, generates the correct response to the command, and then reconnects the keyboard.

![Photo of bus traffic showing the inserted bytes](https://github.com/schlae/RS6KB/blob/main/protocol.png)

The hardware is a small circuit board with a microcontroller and connectors to allow you to plug in the keyboard and a short wire with a plug going to the host RS/6000 machine.

## Fabrication

The board dimensions are 34mm x 24mm. It is a simple two-layer board. If you're in the USA, it's pretty easy and cheap to get it from [Osh Park](https://oshpark.com). Or just toss it in with your next big order from one of the other PCB vendors.

[Schematic](https://github.com/schlae/RS6KB/blob/main/RS6KB.pdf)

[Fab files](https://github.com/schlae/RS6KB/blob/main/fab/RS6KB_Rev1.zip)

[Bill of Materials](https://github.com/schlae/RS6KB/blob/main/RS6KB.csv)

## Assembly

Put down the surface mount components first. I'd recommend installing the programming connector, J3, but don't install the remaining through-hole components until you've successfully programmed the bootloader. Otherwise those parts might get in the way of any solder cleanup work you might need to perform.

The project uses an ATMega328P with the Arduino bootloader, so you'll need to program that next. There are a couple of options, but I've used the Arduino-As-ISP sketch in another Arduino (but be aware that you need to connect a [10uF capacitor between the RST pin on the Arduino and ground](https://forum.arduino.cc/t/arduinoisp-on-uno-requires-10uf-cap-why/102111) for it to work correctly). You can also use something like a USBtinyISP ([AVR Pocket Programmer](https://www.sparkfun.com/products/9825)). Newer versions of the Arduino IDE seem to have an issue programming the lock bits. I used an older version (1.8.16) which worked fine. Program the board as an Arduino Nano or as an Arduino Pro Mini (16MHz).

Once that is done, attach a 6-pin serial adapter such as the [SparkFun FTDI Basic (**5V version**)](https://www.sparkfun.com/products/9716), load the RS6KB.ino sketch, and make sure it downloads correctly.

Then go ahead and install the remaining through-hole parts.

## The Keyboard Cable

Header J2 connects to the RS/6000 through a cable. One end is a 6-pin header, and the other end is a male 6-pin Mini-DIN plug. The pins are connected 1:1 (pin 1 to pin 1, and so on). You could get an in-line Mini-DIN plug and solder it to a short length of wire, or you could sacrifice an old keyboard cable.

It is up to you if you want to solder the wires on the cable directly to the footprint or install a header and connector combination.

Pins 2 and 6 are brought out to header J5 for an optional clicky speaker.

The pinout is reproduced here for reference. It is also inscribed in silkscreen on the back of the board for convenience.

| Pin | Signal         |
| --- | -------------- |
| 1   | Keyboard data  |
| 2   | Speaker signal |
| 3   | Ground         |
| 4   | +5V            |
| 5   | Keyboard clock |
| 6   | Speaker ground |

## Other Notes

I've only tested this with an RS/6000 model 7006 and a genuine IBM Model M keyboard, so I wouldn't be surprised if other keyboards don't work properly. Since I'm not a keyboard collector, I won't be able to help you because it's very unlikely I'll have your exact keyboard, but if you have a USB logic analyzer, you could try doing a little troubleshooting to see where the firmware is getting hung up.
