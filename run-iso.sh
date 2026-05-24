#!/bin/sh
# boot full ISO through GRUB — same path as real hardware
qemu-system-x86_64 -cdrom circular-os.iso -serial stdio -m 64M
