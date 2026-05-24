#!/bin/sh
# boot raw kernel (fast, bypasses GRUB)
qemu-system-x86_64 -kernel circular.elf -serial stdio -display none -m 32M

