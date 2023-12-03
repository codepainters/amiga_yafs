# Firmware

This directory contains the YAFS firmware source code.

In order to build it `avr-gcc` and `avr-libc` are required. On Debian/Ubuntu you can simply:

```sh
$ sudo apt-get install avr-gcc avr-libc make
```

And then:

```sh
$ make
```

When `make` completes, `./out/yafs.elf` is ready for programming. 

Notes:

* build is configured for `ATtiny404`, but `ATtiny416` can be selected (initial prototype was developed for `ATtiny416 Xplained Nano` board). You can switch the MCU by changing `MCU_TYPE` value in the `Makefile`. 

* files under `dev` and `include` directories (device specs, headers and runtime libs) come from Microchip. Currently `avr-gcc` is missing definitions for `ATTiny404/416`.



## Programming

The `ATtiny 404` MCU uses `UPDI` single-pin programming / debugging interface. While debugging requires dedicated interface, programming can be done trivially with:

* USB-to-UART interface (I use cheap Chinese PL2303-based one).

* single 2k2 or 4k7 resistor

* [avrdude](https://github.com/avrdudes/avrdude) programming tool (one available in Ubuntu should work as well, just `sudo apt-get install avrdude`).

  

Connect the UART cable and resistor like this (label colors match my UART cable):

![](/home/czajnik/work/YAFS/firmware/pcb_cia_prog.png)



Then issue the following command (change `/dev/ttyUSB0` to the actual device corresponding to your USB-to-UART cable):

```sh
avrdude -v -F -b 9600 -c serialupdi -P /dev/ttyUSB0 -p attiny404 -U flash:w:out/yafs.hex:i
```

That's it!





