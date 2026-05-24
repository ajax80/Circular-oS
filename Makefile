CC      = gcc
CXX     = g++
AS      = nasm
LD      = ld

CFLAGS  = -m32 -std=c99 -ffreestanding -nostdlib -nostdinc \
          -fno-builtin -fno-stack-protector -Wall -Wextra -Ikernel
CXXFLAGS= -m32 -std=c++11 -ffreestanding -nostdlib -nostdinc \
          -fno-builtin -fno-stack-protector -fno-exceptions -fno-rtti \
          -Wall -Wextra -Ikernel
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld --oformat elf32-i386

OBJS = boot/boot.o \
       kernel/vga.o \
       kernel/fb.o \
       kernel/serial.o \
       kernel/schema.o \
       kernel/mm.o \
       kernel/pci.o \
       kernel/mouse.o \
       kernel/idt.o \
       kernel/pic.o \
       kernel/isr.o \
       kernel/timer.o \
       kernel/audio.o \
       kernel/shell.o \
       kernel/kernel.o

all: circular.elf

circular.elf: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

boot/boot.o: boot/boot.asm
	$(AS) $(ASFLAGS) -o $@ $<

kernel/isr.o: kernel/isr.asm
	$(AS) $(ASFLAGS) -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) circular.elf

.PHONY: all clean
