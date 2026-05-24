#include "schema.h"
#include "serial.h"

static uint8_t popcount(uint32_t v) {
    uint8_t n = 0;
    while (v) { n += (uint8_t)(v & 1); v >>= 1; }
    return n;
}

void schema_init(schema_instance_t *inst, uint32_t pid, uint8_t target) {
    inst->pid        = pid;
    inst->state      = STATE_NEW_PROCESS;
    inst->weight     = 0;
    inst->target_c   = target;
    inst->prev_state = STATE_SETTLED;
    inst->flags      = 0;
}

uint8_t schema_eval(schema_instance_t *inst, uint32_t flags) {
    uint8_t w;
    inst->flags = flags;

    switch (inst->state) {
        case STATE_NEW_PROCESS:
            w = popcount(flags & F8_MASK);
            inst->weight     = w;
            inst->prev_state = STATE_NEW_PROCESS;
            inst->state      = (w >= SHIFT_THRESHOLD_8) ? STATE_FULL_TRUST : STATE_RECOVERY;
            break;

        case STATE_RECOVERY:
            w = popcount(flags & F9_MASK);
            inst->weight = w;
            inst->state  = (w >= SHIFT_THRESHOLD_9) ? STATE_SETTLED : STATE_FRICTION;
            break;

        case STATE_FRICTION:
            w = popcount(flags & F6_MASK);
            inst->weight = w;
            inst->state  = (w >= SHIFT_THRESHOLD_6) ? STATE_RECOVERY : STATE_EXCISED;
            break;

        case STATE_FULL_TRUST:
            if ((flags & F8_MASK) == F8_MASK)
                inst->state = STATE_FUNDAMENTAL;
            else if (flags)
                inst->state = STATE_PERFECT;
            else
                inst->state = STATE_EXCISED;
            break;
    }

    return inst->state;
}

const char *state_name(uint8_t s) {
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

void schema_boot(void) {
    serial_puts("[1] kernel core alive\n");
    serial_puts("[2] init\n");
    serial_puts("[3] runlevel\n");
    serial_puts("[4] env load\n");
    serial_puts("[5] env active\n");
    /* step 6 (STATE_FRICTION) is intentionally absent from clean boot.
       FRICTION has three outcomes: a=failed as expected, b=failed without
       expected end, c=passed without reason. None are valid at power-on.
       If FRICTION appears on boot, the system was compromised before it started.
       Clean boot goes directly to SETTLED — friction is never the starting ground. */
    serial_puts("[7] settled\n");
}
