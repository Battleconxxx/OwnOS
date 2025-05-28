DEFAULT_HOST := $(shell ../default-host.sh)
HOST ?= $(DEFAULT_HOST)
HOSTARCH := $(shell ../target-triplet-to-arch.sh $(HOST))

CFLAGS?=-m32 -O2 -g
CPPFLAGS?=
LDFLAGS?=
LIBS?=

#interrupts stuff
AS=nasm
ASFLAGS=-f elf

DESTDIR?=
PREFIX?=/usr/local
EXEC_PREFIX?=$(PREFIX)
BOOTDIR?=$(EXEC_PREFIX)/boot
INCLUDEDIR?=$(PREFIX)/include
LIBC_OBJS ?=

CFLAGS:=$(CFLAGS) -ffreestanding -Wall -Wextra
CPPFLAGS:=$(CPPFLAGS) -D__is_kernel -Iinclude -I../libc/include
LDFLAGS:=$(LDFLAGS) -L../libc
LIBS:=$(LIBS) -nostdlib -lk -lgcc

ARCHDIR=arch/$(HOSTARCH)

include $(ARCHDIR)/make.config

CFLAGS:=$(CFLAGS) $(KERNEL_ARCH_CFLAGS)
CPPFLAGS:=$(CPPFLAGS) $(KERNEL_ARCH_CPPFLAGS)
LDFLAGS:=$(LDFLAGS) $(KERNEL_ARCH_LDFLAGS)
LIBS:=$(LIBS) $(KERNEL_ARCH_LIBS)


KERNEL_OBJS=\
$(KERNEL_ARCH_OBJS) \
kernel/kernel.o \
memory/memory.o \
interrupts/interrupts.o \
interrupts/idt.o \
interrupts/isr.o \
FS/fs.o \
arch/i386/gdt.o \
arch/i386/gdt_flush.o \
thread/scheduler.o

OBJS=\
$(ARCHDIR)/crti.o \
$(ARCHDIR)/crtbegin.o \
$(KERNEL_OBJS) \
$(LIBC_OBJS) \
$(ARCHDIR)/crtend.o \
$(ARCHDIR)/crtn.o

LINK_LIST=\
$(LDFLAGS) \
$(ARCHDIR)/crti.o \
$(ARCHDIR)/crtbegin.o \
$(KERNEL_OBJS) \
$(LIBC_OBJS) \
$(LIBS) \
$(ARCHDIR)/crtend.o \
$(ARCHDIR)/crtn.o

.PHONY: all clean install install-headers install-kernel
.SUFFIXES: .o .c .S

all: myos.kernel

myos.kernel: $(OBJS) $(ARCHDIR)/linker.ld
	$(CC) -T $(ARCHDIR)/linker.ld -o $@ $(CFLAGS) $(LINK_LIST)
	grub-file --is-x86-multiboot myos.kernel

$(ARCHDIR)/crtbegin.o $(ARCHDIR)/crtend.o:
	OBJ=`$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(@F)` && cp "$$OBJ" $@

.c.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

.S.o:
	$(CC) -m32 -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

#gdt stuff
arch/i386/gdt.o: arch/i386/gdt.c include/kernel/gdt.h
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

arch/i386/gdt_flush.o: arch/i386/gdt.asm
	$(AS) $(ASFLAGS) $< -o $@

#interrupts compiling
interrupts/idt.o: interrupts/idt.asm
	$(AS) $(ASFLAGS) $< -o $@

interrupts/isr.o: interrupts/isr.asm
	$(AS) $(ASFLAGS) $< -o $@

clean:
	rm -f myos.kernel
	rm -f $(OBJS) *.o */*.o */*/*.o
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d

install: install-headers install-kernel

install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/.

install-kernel: myos.kernel
	mkdir -p $(DESTDIR)$(BOOTDIR)
	cp myos.kernel $(DESTDIR)$(BOOTDIR)

-include $(OBJS:.o=.d)
