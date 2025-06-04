#!/bin/bash

# Go to kernel directory
cd kernel/ || { echo "Failed to cd into kernel/"; exit 1; }

# Clean previous builds
make clean

# Build the kernel
make

# Return to the parent directory
cd ..

# Run QEMU with debugging enabled
qemu-system-i386 -kernel kernel/kernel.elf -S -gdb tcp::1234 -serial stdio
