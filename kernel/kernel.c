#include "schema.h"
#include "serial.h"

static const char *state_name(uint8_t s) {
    switch (s) {
        case STATE_FUNDAMENTAL: return "FUNDAMENTAL";
        case STATE_FRICTION:    return "FRICTION";
        case STATE_SETTLED:     return "SETTLED";
        case STATE_NEW_PROCESS: return "NEW_PROCESS";
        case STATE_RECOVERY:    return "RECOVERY";
        case STATE_FULL_TRUST:  return "FULL_TRUST";
        case STATE_PERFECT:     return "PERFECT";
        case STATE_EXCISED:     return "EXCISED";
        default:                return "UNKNOWN";
    }
}

static void report(schema_instance_t *inst) {
    serial_puts("  pid=");
    serial_putu(inst->pid);
    serial_puts(" state=");
    serial_puts(state_name(inst->state));
    serial_puts(" weight=");
    serial_putu(inst->weight);
    serial_puts("/target=");
    serial_puts(state_name(inst->target_c));
    serial_putc('\n');
}

static void spawn(schema_instance_t *inst, uint32_t pid, const char *label) {
    schema_init(inst, pid, STATE_PERFECT);
    serial_puts("\n--- ");
    serial_puts(label);
    serial_puts(" ---\n");
    report(inst);
}

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

    /* Kernel root instance — the substrate */
    schema_instance_t root;
    schema_init(&root, 0, STATE_FUNDAMENTAL);
    root.state = STATE_FUNDAMENTAL;
    serial_puts("kernel: ");
    report(&root);

    /* pid 1 — all conditions met, G02 arc — 8→10→1 */
    schema_instance_t p1;
    spawn(&p1, 1, "pid 1: all conditions (G02)");
    schema_eval(&p1, F8_MASK);
    serial_puts("  eval 8/8 -> ");
    report(&p1);
    schema_eval(&p1, F8_MASK);
    serial_puts("  trust  -> ");
    report(&p1);

    /* pid 2 — partial, reroutes through recovery (G03) */
    schema_instance_t p2;
    spawn(&p2, 2, "pid 2: partial conditions (G03 reroute)");
    schema_eval(&p2, F8_HW_EXISTS | F8_DEP_PRESENT | F8_MEM_AVAIL);
    serial_puts("  eval 3/8 -> ");
    report(&p2);
    schema_eval(&p2, F9_RETRY_COUNT | F9_RETRY_WIN | F9_FALL_EXISTS | F9_FALL_HEALTH |
                     F9_MEM_FREE   | F9_MEM_SUFF   | F9_ESC_PATH   | F9_ESC_AUTH);
    serial_puts("  recv 8/12 -> ");
    report(&p2);

    /* pid 3 — falls through 9 and 6, gets 76'd */
    schema_instance_t p3;
    spawn(&p3, 3, "pid 3: fails all recovery (76)");
    schema_eval(&p3, F8_HW_EXISTS | F8_DEP_PRESENT);
    serial_puts("  eval 2/8 -> ");
    report(&p3);
    schema_eval(&p3, F9_RETRY_COUNT | F9_RETRY_WIN);
    serial_puts("  recv 2/12 -> ");
    report(&p3);
    schema_eval(&p3, F6_ERR_PATH | F6_ERR_RES);
    serial_puts("  fric 2/6 -> ");
    report(&p3);

    serial_puts("\nState 7 -- settled\n");

    for (;;) __asm__("hlt");
}
