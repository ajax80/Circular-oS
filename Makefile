CC      = gcc
AS      = nasm
LD      = ld

CFLAGS  = -m32 -std=c99 -ffreestanding -nostdlib -nostdinc \
          -fno-builtin -fno-stack-protector -Wall -Wextra -Ikernel
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld --oformat elf32-i386

OBJS = boot/boot.o \
       kernel/serial.o \
       kernel/schema.o \
       kernel/kernel.o

all: circular.elf

circular.elf: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

boot/boot.o: boot/boot.asm
	$(AS) $(ASFLAGS) -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) circular.elf

.PHONY: all clean
