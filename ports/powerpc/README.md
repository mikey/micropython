# The minimal port

This port is intended to be a minimal MicroPython port that runs in
mambo, microwatt simulator with ghdl or microwatt on Xilinx FPGA with
potato UART.

## Building

By default the port will be built for the host machine:

    $ make

## Cross compilation for POWERPC

The Makefile has the ability to build for a POWERPC

    $ make CROSS=1

Building will produce the build/firmware.bin file which can be used
mambo or microwatt
