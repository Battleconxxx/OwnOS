#!/bin/bash
# qemu_debug.sh - run QEMU with GDB stub and serial output

KERNEL_BIN="kernel/kernel.elf"   # change path if needed

# Clean and build kernel before running, optional

cd kernel/

make clean && make

cd ..

# Run QEMU with:
# -kernel : load kernel ELF/binary
# -serial stdio : redirect guest serial to your terminal
# -s : enable gdbserver on tcp:1234
# -S : freeze CPU at start (wait for GDB attach)
qemu-system-i386 -kernel kernel/kernel.elf -s -S
