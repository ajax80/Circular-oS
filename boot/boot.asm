; Circular OS — bootstrap
; Multiboot1 entry. GRUB/QEMU loads us into 32-bit protected mode.
; We set up the stack and call into C.

MB2_MAGIC equ 0xE85250D6
MB2_ARCH  equ 0

section .multiboot
align 8
mb2_hdr_start:
    dd MB2_MAGIC
    dd MB2_ARCH
    dd (mb2_hdr_end - mb2_hdr_start)
    dd -(MB2_MAGIC + MB2_ARCH + (mb2_hdr_end - mb2_hdr_start))

    align 8                 ; tag: framebuffer request (type 5)
    dw 5
    dw 1                    ; optional — don't fail if unavailable
    dd 20
    dd 0                    ; width  (0 = any)
    dd 0                    ; height (0 = any)
    dd 0                    ; depth  (0 = any)

    align 8                 ; tag: end (type 0)
    dw 0
    dw 0
    dd 8
mb2_hdr_end:

section .data
saved_eax: dd 0
saved_ebx: dd 0

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

section .text
bits 32
global _start
extern kernel_main
extern __bss_start
extern __bss_end

_start:
    mov  dword [0xB8000], 0x4F434F43
    mov  [saved_eax], eax
    mov  [saved_ebx], ebx
    mov  esp, stack_top

    ; zero BSS — real hardware has dirty RAM; QEMU zeroes it silently
    mov  edi, __bss_start
    mov  ecx, __bss_end
    sub  ecx, edi
    xor  eax, eax
    rep  stosb

    mov  eax, [saved_eax]
    mov  ebx, [saved_ebx]
    push 0
    popf
    push ebx
    push eax
    call kernel_main
    cli
.halt:
    hlt
    jmp .halt
