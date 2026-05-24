; Circular OS — bootstrap
; Multiboot1 entry. GRUB/QEMU loads us into 32-bit protected mode.
; We set up the stack and call into C.

MAGIC    equ 0x1BADB002
FLAGS    equ 0x0
CHECKSUM equ -(MAGIC + FLAGS)

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

section .text
bits 32
global _start
extern kernel_main

_start:
    mov  esp, stack_top
    push 0
    popf
    push ebx
    push eax
    call kernel_main
    cli
.halt:
    hlt
    jmp .halt
