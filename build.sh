#!/bin/bash

avr-gcc -Os -DDEBUG_BOUNCE -mmcu=atmega328 -S power.c -o power.s
avr-as -mmcu=atmega328 power.s -o power.o
avr-gcc -Os -mmcu=atmega328 power.o simple_usart/uart.o -o power.x
avr-objcopy -j .text -j .data -O ihex power.x power.hex

avrdude -C${ARDUINO_PATH}/hardware/tools/avr/etc/avrdude.conf -v -patmega328p -carduino -P/dev/ttyACM0 -b115200 -D -Uflash:w:power.hex:i
