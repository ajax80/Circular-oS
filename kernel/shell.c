#include "shell.h"
#include "schema.h"
#include "serial.h"
#include "fb.h"
#include "mm.h"
#include "pci.h"
#include "mouse.h"
#include "audio.h"
#include "timer.h"

#define COL_OUTPUT  0x00CC33
#define COL_PROMPT  0x00FF88
#define COL_STATE   0xFFAA00
#define COL_ERR     0xFF4444
#define COL_76      0xFF6600

#define MAX_PROCS   49
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

static unsigned int state_color(uint8_t s) {
    switch (s) {
        case STATE_FUNDAMENTAL: return 0xCC00FF;  /* purple  — OK, go        */
        case STATE_FULL_TRUST:  return 0x9900CC;  /* deep purple — cleared   */
        case STATE_PERFECT:     return 0xFF00FF;  /* magenta — 88            */
        case STATE_SETTLED:     return 0xFF2244;  /* red     — stable, idle  */
        case STATE_NEW_PROCESS: return 0x00FF88;  /* green   — young, in arc */
        case STATE_RECOVERY:    return 0x0088FF;  /* blue    — needs attn    */
        case STATE_FRICTION:    return 0xFFDD00;  /* yellow  — about to fail */
        case STATE_EXCISED:     return 0xFFFFFF;  /* white   — down, broke   */
        default:                return 0x222222;
    }
}

static char state_glyph(uint8_t s) {
    switch (s) {
        case STATE_FUNDAMENTAL: return 'F';
        case STATE_FULL_TRUST:  return 'T';
        case STATE_PERFECT:     return '8';
        case STATE_SETTLED:     return 'S';
        case STATE_NEW_PROCESS: return 'N';
        case STATE_RECOVERY:    return 'R';
        case STATE_FRICTION:    return '~';
        case STATE_EXCISED:     return 'X';
        default:                return '?';
    }
}

static void cmd_status(void) {
    int i, fund=0, recov=0, ex=0;
    for (i = 0; i < MAX_PROCS; i++) {
        if (!procs[i].active) continue;
        switch (procs[i].inst.state) {
            case STATE_FUNDAMENTAL: fund++;  break;
            case STATE_FULL_TRUST:
            case STATE_PERFECT:     break;
            case STATE_RECOVERY:
            case STATE_FRICTION:    recov++; break;
            case STATE_EXCISED:     ex++;    break;
        }
    }

    fb_set_fg(0x00FF41);  serial_putu((uint32_t)fund);
    fb_set_fg(0xAAAAAA);  serial_puts(" fund  ");
    fb_set_fg(0xFF8800);  serial_putu((uint32_t)recov);
    fb_set_fg(0xAAAAAA);  serial_puts(" recov  ");
    fb_set_fg(0x666666);  serial_putu((uint32_t)ex);
    fb_set_fg(0xAAAAAA);  serial_puts(" excised\n\n");

    for (i = 0; i < MAX_PROCS; i++) {
        if (i > 0 && i % 7 == 0) serial_putc('\n');
        if (procs[i].active) {
            fb_set_fg(state_color(procs[i].inst.state));
            serial_puts(" [");
            if (i + 1 < 10) serial_putc(' ');
            serial_putu((uint32_t)(i + 1));
            serial_putc(':');
            serial_putc(state_glyph(procs[i].inst.state));
            serial_putc(']');
        } else {
            fb_set_fg(0x2A2A2A);
            serial_puts(" [ -- ]");
        }
    }
    fb_set_fg(COL_OUTPUT);
    serial_puts("\n");
}

static void run_arc(schema_instance_t *inst, uint32_t flags) {
    int iters = 0;
    while (iters++ < 8) {
        uint8_t cur = inst->state;
        if (cur == STATE_NEW_PROCESS || cur == STATE_FULL_TRUST ||
            cur == STATE_RECOVERY    || cur == STATE_FRICTION)
            schema_eval(inst, flags);
        else break;
    }
}

static void cmd_bootleg(void) {
    static const uint32_t node_flags[49] = {
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFE,0xFC,0xF8,0xF0,0xE0,0xC0,0x80,
        0x7F,0x3F,0x1F,0x0F,0x07,0x03,0x00
    };
    int i, j, spawned = 0;

    for (i = 0; i < MAX_PROCS; i++) procs[i].active = 0;
    next_pid = 1;

    fb_set_fg(0x00FF41); serial_puts("booting federation");
    fb_set_fg(COL_OUTPUT);

    for (i = 0; i < 49; i++) {
        int slot = -1;
        for (j = 0; j < MAX_PROCS; j++) {
            if (!procs[j].active) { slot = j; break; }
        }
        if (slot < 0) break;
        schema_init(&procs[slot].inst, next_pid++, STATE_PERFECT);
        procs[slot].active = 1;
        run_arc(&procs[slot].inst, node_flags[i]);
        if (procs[slot].inst.state == STATE_EXCISED)
            procs[slot].active = 0;
        else
            spawned++;
        serial_putc('.');
    }

    serial_putc('\n');
    fb_set_fg(0x00FF41);  serial_putu((uint32_t)spawned);
    fb_set_fg(COL_OUTPUT); serial_puts("/49 nodes online\n");
    fb_update_state("FEDERATED");
}

static void cmd_clear(void) {
    int i;
    for (i = 0; i < 40; i++) serial_putc('\n');
}

static void cmd_echo(const char *s) {
    serial_puts(s);
    serial_putc('\n');
}

static void cmd_mem(void) {
    uint32_t used  = mm_used();
    uint32_t total = mm_total();
    serial_puts("  heap used:  "); serial_putu(used);  serial_puts(" bytes\n");
    serial_puts("  heap total: "); serial_putu(total); serial_puts(" bytes\n");
    serial_puts("  heap free:  "); serial_putu(total - used); serial_puts(" bytes\n");
}

static const char *pci_cls_name(uint8_t c) {
    switch (c) {
        case 0x00: return "legacy";
        case 0x01: return "storage";
        case 0x02: return "network";
        case 0x03: return "display";
        case 0x04: return "multimedia";
        case 0x06: return "bridge";
        case 0x0C: return "serial bus";
        default:   return "device";
    }
}

static void cmd_lspci(void) {
    int i;
    if (!pci_count) { serial_puts("  no PCI devices found\n"); return; }
    for (i = 0; i < pci_count; i++) {
        pci_dev_t *d = &pci_devs[i];
        serial_puts("  ");
        serial_putu(d->bus); serial_putc(':');
        serial_putu(d->dev); serial_putc('.');
        serial_putu(d->func);
        serial_puts("  ");
        serial_puth(d->vendor); serial_putc(':');
        serial_puth(d->device);
        serial_puts("  ");
        serial_puts(pci_cls_name(d->class));
        serial_putc('\n');
    }
}

static void cmd_arch(void) {
    uint32_t eax, ebx, ecx, edx;
    char brand[48];
    int i;

    __asm__ volatile("cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0));

    /* vendor string is in EBX:EDX:ECX */
    *(uint32_t *)(brand)      = ebx;
    *(uint32_t *)(brand + 4)  = edx;
    *(uint32_t *)(brand + 8)  = ecx;
    brand[12] = 0;
    serial_puts("  vendor: "); serial_puts(brand); serial_putc('\n');

    __asm__ volatile("cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1));
    serial_puts("  family: "); serial_putu((eax >> 8) & 0xF);
    serial_puts("  model: ");  serial_putu((eax >> 4) & 0xF);
    serial_puts("  step: ");   serial_putu(eax & 0xF);
    serial_putc('\n');
    serial_puts("  features:");
    if (edx & (1 << 25)) serial_puts(" SSE");
    if (edx & (1 << 26)) serial_puts(" SSE2");
    if (ecx & (1 <<  0)) serial_puts(" SSE3");
    if (ecx & (1 << 28)) serial_puts(" AVX");
    if (edx & (1 <<  5)) serial_puts(" MSR");
    if (edx & (1 << 23)) serial_puts(" MMX");
    serial_putc('\n');

    /* brand string (EAX 0x80000002-4) */
    uint32_t mx;
    __asm__ volatile("cpuid" : "=a"(mx) : "a"(0x80000000u) : "ebx","ecx","edx");
    if (mx >= 0x80000004u) {
        uint32_t *p = (uint32_t *)brand;
        uint32_t leaf;
        for (leaf = 0x80000002u; leaf <= 0x80000004u; leaf++) {
            __asm__ volatile("cpuid"
                : "=a"(p[0]),"=b"(p[1]),"=c"(p[2]),"=d"(p[3])
                : "a"(leaf));
            p += 4;
        }
        brand[47] = 0;
        for (i = 0; brand[i] == ' '; i++);
        serial_puts("  cpu: "); serial_puts(brand + i); serial_putc('\n');
    }
}

static int schema_corrupt_level(void) {
    int i, friction = 0, recov = 0, excised = 0, total = 0;
    for (i = 0; i < MAX_PROCS; i++) {
        if (!procs[i].active) continue;
        total++;
        switch (procs[i].inst.state) {
            case STATE_FRICTION:    friction++; break;
            case STATE_RECOVERY:    recov++;    break;
            case STATE_EXCISED:     excised++;  break;
            default: break;
        }
    }
    if (!total) return 0;
    if (excised > total / 4) return 3;   /* >25% excised  — dropout     */
    if (recov   > total / 4) return 2;   /* >25% recovery — wrong notes */
    if (friction > 0)        return 1;   /* any friction  — timing drift */
    return 0;
}

static void cmd_groove(void) {
    int level = schema_corrupt_level();
    audio_set_corruption(level);
    audio_start_groove();
    serial_puts("groove: corruption level ");
    serial_putu((uint32_t)level);
    serial_putc('\n');
    switch (level) {
        case 0: serial_puts("  clean — all nodes nominal\n");     break;
        case 1: serial_puts("  drift — friction detected\n");     break;
        case 2: serial_puts("  wrong notes — recovery nodes\n");  break;
        case 3: serial_puts("  dropout — cascade failure\n");     break;
    }
}

static void cmd_stopgroove(void) {
    audio_stop_groove();
    serial_puts("groove stopped\n");
}

static void cmd_tone(const char *p) {
    uint32_t freq = parse_uint(p);
    while (*p && *p != ' ') p++;
    uint32_t ms   = parse_uint(p);
    if (!freq) freq = 440;
    if (!ms)   ms   = 500;
    audio_tone(freq, ms);
    serial_puts("tone ");
    serial_putu(freq);
    serial_puts("Hz ");
    serial_putu(ms);
    serial_puts("ms queued\n");
}

static void cmd_beep(void) {
    audio_tone(880, 100);
    audio_silence(50);
    audio_tone(1046, 150);
}

static void cmd_schema_audio(uint32_t pid) {
    int i;
    for (i = 0; i < MAX_PROCS; i++) {
        if (procs[i].active && procs[i].inst.pid == pid) {
            audio_play_schema(procs[i].inst.state);
            serial_puts("playing signature for pid ");
            serial_putu(pid);
            serial_puts(" state=");
            serial_puts(state_name(procs[i].inst.state));
            serial_putc('\n');
            return;
        }
    }
    serial_puts("pid not found\n");
}

static void cmd_help(void) {
    serial_puts("commands:\n");
    serial_puts("  spawn <hex>       spawn process with condition flags\n");
    serial_puts("  eval <pid> <hex>  push flags to existing process\n");
    serial_puts("  ps                list active processes\n");
    serial_puts("  kill <pid>        excise a process (76)\n");
    serial_puts("  reset             clear all processes\n");
    serial_puts("  bootleg           boot all 49 federation nodes\n");
    serial_puts("  status            live federation map\n");
    serial_puts("  clear             clear terminal\n");
    serial_puts("  echo <text>       print text\n");
    serial_puts("  mem               heap memory stats\n");
    serial_puts("  lspci             list PCI devices\n");
    serial_puts("  arch              CPU info\n");
    serial_puts("  groove            start schema-driven audio loop\n");
    serial_puts("  stop              stop groove\n");
    serial_puts("  tone <hz> <ms>    play a tone\n");
    serial_puts("  beep              test beep\n");
    serial_puts("  sig <pid>         play schema signature for process\n");
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
    } else if (p[0]=='s' && p[1]=='t') {
        cmd_status();
    } else if (p[0]=='b' && p[1]=='o') {
        cmd_bootleg();
    } else if (p[0]=='c' && p[1]=='l') {
        cmd_clear();
    } else if (p[0]=='e' && p[1]=='c') {
        cmd_echo(skip_spaces(p + 4));
    } else if (p[0]=='m' && p[1]=='e') {
        cmd_mem();
    } else if (p[0]=='l' && p[1]=='s') {
        cmd_lspci();
    } else if (p[0]=='a' && p[1]=='r') {
        cmd_arch();
    } else if (p[0]=='g' && p[1]=='r') {
        cmd_groove();
    } else if (p[0]=='s' && p[1]=='t' && p[2]=='o') {
        cmd_stopgroove();
    } else if (p[0]=='t' && p[1]=='o') {
        cmd_tone(skip_spaces(p + 4));
    } else if (p[0]=='b' && p[1]=='e') {
        cmd_beep();
    } else if (p[0]=='s' && p[1]=='i') {
        cmd_schema_audio(parse_uint(p + 3));
    } else if (p[0]=='h') {
        cmd_help();
    } else {
        serial_puts("unknown command\n");
    }
}

static int  fed_online = 0;
static int  fed_total  = 49;

static int count_online(void) {
    int i, n = 0;
    for (i = 0; i < MAX_PROCS; i++)
        if (procs[i].active) n++;
    return n;
}

void shell_run(void) {
    char buf[BUF_LEN];
    int  pos = 0;
    int  cur_x, cur_y, last_x = -1, last_y = -1;
    int  first = 1;

    /* init mouse to screen centre */
    if (fb_active()) {
        int w = fb_get_w(), h = fb_get_h();
        if (w && h) mouse_set_bounds(w, h);
    }
    mouse_init();

    fb_set_fg(0xFFFFFF);
    serial_puts("\nCircular OS shell\n");
    fb_set_fg(COL_OUTPUT);
    serial_puts("type 'help' for commands\n\n");

    fb_set_fg(COL_PROMPT);
    serial_puts("circular> ");
    fb_set_fg(COL_OUTPUT);

    while (1) {
        /* mouse */
        mouse_poll();
        cur_x = mouse_x;
        cur_y = mouse_y;
        if (fb_active() && (cur_x != last_x || cur_y != last_y)) {
            fb_erase_cursor();
            fb_draw_cursor(cur_x, cur_y);
            last_x = cur_x;
            last_y = cur_y;
        }

        /* taskbar refresh on first run or after commands */
        if (first && fb_active()) {
            fed_online = count_online();
            fb_draw_taskbar("SETTLED", fed_online, fed_total, cur_x, cur_y);
            first = 0;
        }

        /* keyboard (non-blocking) */
        int c = serial_trygetc();
        if (!c) continue;

        if (c == '\n' || c == '\r') {
            buf[pos] = 0;
            serial_putc('\n');
            dispatch(buf);
            pos = 0;
            /* refresh taskbar after every command */
            if (fb_active()) {
                fed_online = count_online();
                fb_draw_taskbar("SETTLED", fed_online, fed_total, cur_x, cur_y);
            }
            fb_set_fg(COL_PROMPT);
            serial_puts("circular> ");
            fb_set_fg(COL_OUTPUT);
        } else if (c == '\b') {
            if (pos > 0) { pos--; serial_puts("\b \b"); }
        } else if (pos < BUF_LEN - 1) {
            buf[pos++] = (char)c;
            serial_putc((char)c);
        }
    }
}
