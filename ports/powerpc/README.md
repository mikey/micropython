# The minimal port

This port is intended to be a minimal MicroPython port that runs in
QEMU, microwatt simulator with ghdl or microwatt on Xilinx FPGA with
potato UART.

## Building

By default the port will be built for the host machine:

    $ make

## Cross compilation for POWERPC

The Makefile has the ability to build for a POWERPC

    $ make CROSS=1

Building will produce the build/firmware.bin file which can be used
QEMU or microwatt

To run in QEMU use:

    $ ./qemu-system-ppc64 -M powernv -cpu POWER9 -nographic \
       -bios build/firmware.bin
