#include "shell.h"
#include "schema.h"
#include "serial.h"
#include "fb.h"

#define COL_OUTPUT  0x00CC33
#define COL_PROMPT  0x00FF88
#define COL_STATE   0xFFAA00
#define COL_ERR     0xFF4444
#define COL_76      0xFF6600

#define MAX_PROCS   16
#define BUF_LEN     64

typedef struct {
    schema_instance_t inst;
    uint8_t active;
} proc_entry_t;

static proc_entry_t procs[MAX_PROCS];
static uint32_t     next_pid = 1;

static const char *skip_spaces(const char *s) {
    while (*s == ' ') s++;
    return s;
}

static uint32_t parse_hex(const char *s) {
    uint32_t v = 0;
    s = skip_spaces(s);
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
    while (*s) {
        char c = *s++;
        if      (c >= '0' && c <= '9') v = (v << 4) | (uint32_t)(c - '0');
        else if (c >= 'a' && c <= 'f') v = (v << 4) | (uint32_t)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') v = (v << 4) | (uint32_t)(c - 'A' + 10);
        else break;
    }
    return v;
}

static uint32_t parse_uint(const char *s) {
    uint32_t v = 0;
    s = skip_spaces(s);
    while (*s >= '0' && *s <= '9') v = v * 10 + (uint32_t)(*s++ - '0');
    return v;
}

static void print_inst(schema_instance_t *inst) {
    serial_puts("  pid=");
    serial_putu(inst->pid);
    serial_puts(" state=");
    serial_puts(state_name(inst->state));
    serial_puts(" weight=");
    serial_putu(inst->weight);
    serial_putc('\n');
}

static void arc_step(schema_instance_t *inst, uint32_t flags, uint8_t max, uint8_t threshold) {
    serial_puts("  [");
    serial_putu(inst->state);
    serial_puts("] weight=");
    serial_putu(inst->weight);
    serial_putc('/');
    serial_putu(max);
    serial_puts(" thr=");
    serial_putu(threshold);
    serial_puts(" -> ");
    serial_puts(state_name(schema_eval(inst, flags)));
    serial_putc('\n');
}

static void cmd_spawn(uint32_t flags) {
    int slot = -1, i;
    for (i = 0; i < MAX_PROCS; i++) {
        if (!procs[i].active) { slot = i; break; }
    }
    if (slot < 0) { serial_puts("process table full\n"); return; }

    schema_instance_t *inst = &procs[slot].inst;
    schema_init(inst, next_pid++, STATE_PERFECT);
    procs[slot].active = 1;

    serial_puts("spawned pid=");
    serial_putu(inst->pid);
    serial_puts(" flags=");
    serial_puth(flags);
    serial_putc('\n');

    int iters = 0;
    while (iters++ < 8) {
        uint8_t cur = inst->state;
        if (cur == STATE_NEW_PROCESS)
            arc_step(inst, flags, STATE_8_MAX, SHIFT_THRESHOLD_8);
        else if (cur == STATE_FULL_TRUST)
            arc_step(inst, flags, STATE_8_MAX, STATE_8_MAX);
        else if (cur == STATE_RECOVERY)
            arc_step(inst, flags, STATE_9_MAX, SHIFT_THRESHOLD_9);
        else if (cur == STATE_FRICTION)
            arc_step(inst, flags, STATE_6_MAX, SHIFT_THRESHOLD_6);
        else break;
    }

    if (inst->state == STATE_EXCISED) {
        procs[slot].active = 0;
        fb_set_fg(COL_76);
        serial_puts("  76: branch excised\n");
        fb_set_fg(COL_OUTPUT);
    }
}

static void cmd_eval(uint32_t pid, uint32_t flags) {
    int i;
    for (i = 0; i < MAX_PROCS; i++) {
        if (procs[i].active && procs[i].inst.pid == pid) {
            schema_instance_t *inst = &procs[i].inst;
            serial_puts("eval pid=");
            serial_putu(pid);
            serial_puts(" flags=");
            serial_puth(flags);
            serial_putc('\n');
            schema_eval(inst, flags);
            print_inst(inst);
            if (inst->state == STATE_EXCISED) procs[i].active = 0;
            return;
        }
    }
    serial_puts("pid not found\n");
}

static void cmd_ps(void) {
    int i, found = 0;
    for (i = 0; i < MAX_PROCS; i++) {
        if (procs[i].active) {
            print_inst(&procs[i].inst);
            found = 1;
        }
    }
    if (!found) serial_puts("  no active processes\n");
}

static void cmd_kill(uint32_t pid) {
    int i;
    for (i = 0; i < MAX_PROCS; i++) {
        if (procs[i].active && procs[i].inst.pid == pid) {
            procs[i].inst.state  = STATE_EXCISED;
            procs[i].inst.weight = 0;
            procs[i].active = 0;
            serial_puts("76: pid=");
            serial_putu(pid);
            serial_puts(" excised\n");
            return;
        }
    }
    serial_puts("pid not found\n");
}

static void cmd_reset(void) {
    int i;
    for (i = 0; i < MAX_PROCS; i++) procs[i].active = 0;
    next_pid = 1;
    serial_puts("process table cleared\n");
}

static void cmd_help(void) {
    serial_puts("commands:\n");
    serial_puts("  spawn <hex>       spawn process with condition flags\n");
    serial_puts("  eval <pid> <hex>  push flags to existing process\n");
    serial_puts("  ps                list active processes\n");
    serial_puts("  kill <pid>        excise a process (76)\n");
    serial_puts("  reset             clear all processes\n");
    serial_puts("  help              this\n");
    serial_puts("\nflag reference (state 8 / I vector):\n");
    serial_puts("  0x01 hw_exists  0x02 hw_responds  0x04 dep_present  0x08 dep_state\n");
    serial_puts("  0x10 mem_avail  0x20 mem_safe     0x40 perm_present 0x80 perm_auth\n");
    serial_puts("  0xff all pass -> FULL_TRUST -> FUNDAMENTAL\n");
}

static void dispatch(char *line) {
    const char *p = skip_spaces(line);
    if (!*p) return;

    if (p[0]=='s' && p[1]=='p' && p[2]=='a' && p[3]=='w' && p[4]=='n') {
        cmd_spawn(parse_hex(p + 5));
    } else if (p[0]=='e' && p[1]=='v' && p[2]=='a' && p[3]=='l') {
        p = skip_spaces(p + 4);
        uint32_t pid = parse_uint(p);
        while (*p && *p != ' ') p++;
        cmd_eval(pid, parse_hex(p));
    } else if (p[0]=='p' && p[1]=='s' && (p[2]==' ' || p[2]=='\0')) {
        cmd_ps();
    } else if (p[0]=='k' && p[1]=='i' && p[2]=='l' && p[3]=='l') {
        cmd_kill(parse_uint(p + 4));
    } else if (p[0]=='r' && p[1]=='e' && p[2]=='s' && p[3]=='e' && p[4]=='t') {
        cmd_reset();
    } else if (p[0]=='h') {
        cmd_help();
    } else {
        serial_puts("unknown command — type help\n");
    }
}

void shell_run(void) {
    char buf[BUF_LEN];
    fb_set_fg(0xFFFFFF);
    serial_puts("\nCircular OS shell\n");
    fb_set_fg(COL_OUTPUT);
    serial_puts("type 'help' for commands\n\n");
    while (1) {
        fb_set_fg(COL_PROMPT);
        serial_puts("circular> ");
        fb_set_fg(COL_OUTPUT);
        serial_gets(buf, BUF_LEN);
        dispatch(buf);
    }
}
