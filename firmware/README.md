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