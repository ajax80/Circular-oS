#include "schema.h"
#include "serial.h"
#include "shell.h"

void kernel_main(unsigned int magic, unsigned int mb_info) {
    (void)mb_info;

    serial_init();
    serial_puts("\nCircular OS\n");
    serial_puts("===========\n");

    if (magic != 0x2BADB002) {
        serial_puts("bad multiboot magic\n");
        for (;;) __asm__("hlt");
    }

    schema_boot();
    shell_run();
}
