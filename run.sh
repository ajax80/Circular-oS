#!/bin/sh
qemu-system-x86_64 -kernel circular.elf -serial stdio -display none -m 32M
