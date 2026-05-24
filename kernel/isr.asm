bits 32
section .text

extern irq0_handler

global irq0_stub
irq0_stub:
    pusha
    call irq0_handler
    popa
    iret
