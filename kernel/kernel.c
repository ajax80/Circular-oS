#include "schema.h"
#include "serial.h"
#include "vga.h"
#include "fb.h"
#include "shell.h"

void kernel_main(unsigned int magic, unsigned int mb_info_addr) {
    vga_init();
    serial_init();
    serial_puts("Circular OS\n===========\n");

    if (magic != 0x36d76289) {
        serial_puts("bad multiboot magic\n");
        for (;;) __asm__("hlt");
    }

    /* multiboot2: walk tags to find framebuffer (type 8) */
    {
        unsigned char *tag = (unsigned char *)(mb_info_addr + 8);
        unsigned int fb_addr = 0, fb_pitch = 0, fb_w = 0, fb_h = 0;
        unsigned char fb_bpp = 0, fb_type = 0;
        while (1) {
            unsigned int t    = *(unsigned int *)tag;
            unsigned int size = *(unsigned int *)(tag + 4);
            if (t == 0) break;
            if (t == 8) {
                fb_addr  = *(unsigned int *)(tag + 8);
                fb_pitch = *(unsigned int *)(tag + 16);
                fb_w     = *(unsigned int *)(tag + 20);
                fb_h     = *(unsigned int *)(tag + 24);
                fb_bpp   = tag[28];
                fb_type  = tag[29];
                serial_puts("fb: addr="); serial_puth(fb_addr);
                serial_puts(" "); serial_putu(fb_w);
                serial_puts("x"); serial_putu(fb_h);
                serial_puts(" bpp="); serial_putu(fb_bpp);
                serial_puts(" type="); serial_putu(fb_type);
                serial_puts("\n");
                break;
            }
            tag += (size + 7) & ~7u;
        }
        if (fb_type == 2 && fb_addr) {
            vga_set_buffer(fb_addr);
        } else if (fb_addr && fb_w && fb_h && fb_bpp >= 15) {
            fb_init(fb_addr, fb_pitch, fb_w, fb_h, fb_bpp);
            fb_draw_header("BOOTING");
        } else {
            serial_puts("fb: none found\n");
        }
    }

    schema_boot();
    fb_update_state("SETTLED");
    shell_run();
}
