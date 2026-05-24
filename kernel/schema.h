#ifndef SCHEMA_H
#define SCHEMA_H

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

/* States */
#define STATE_FUNDAMENTAL   1
#define STATE_FRICTION      6
#define STATE_SETTLED       7
#define STATE_NEW_PROCESS   8
#define STATE_RECOVERY      9
#define STATE_FULL_TRUST    10
#define STATE_PERFECT       88
#define STATE_EXCISED       76

/* Weight maxima per state */
#define STATE_8_MAX         8
#define STATE_9_MAX         12
#define STATE_6_MAX         6

/* Shift thresholds — Jonathan's calibration 2026-05-23 */
#define SHIFT_THRESHOLD_8   5
#define SHIFT_THRESHOLD_9   8
#define SHIFT_THRESHOLD_6   3

/* State 8 condition flags — I vector (8 bits) */
#define F8_HW_EXISTS        (1 << 0)
#define F8_HW_RESPONDS      (1 << 1)
#define F8_DEP_PRESENT      (1 << 2)
#define F8_DEP_STATE        (1 << 3)
#define F8_MEM_AVAIL        (1 << 4)
#define F8_MEM_SAFE         (1 << 5)
#define F8_PERM_PRESENT     (1 << 6)
#define F8_PERM_AUTH        (1 << 7)
#define F8_MASK             0xFF

/* State 9 condition flags — J vector (12 bits) */
#define F9_RETRY_COUNT      (1 << 0)
#define F9_RETRY_WIN        (1 << 1)
#define F9_FALL_EXISTS      (1 << 2)
#define F9_FALL_HEALTH      (1 << 3)
#define F9_MEM_FREE         (1 << 4)
#define F9_MEM_SUFF         (1 << 5)
#define F9_ESC_PATH         (1 << 6)
#define F9_ESC_AUTH         (1 << 7)
#define F9_TIMEOUT_WIN      (1 << 8)
#define F9_TIMEOUT_EXT      (1 << 9)
#define F9_PARTIAL_LOAD     (1 << 10)
#define F9_PARTIAL_MIN      (1 << 11)
#define F9_MASK             0xFFF

/* State 6 condition flags — K vector (6 bits) */
#define F6_ERR_PATH         (1 << 0)
#define F6_ERR_RES          (1 << 1)
#define F6_ROLL_STATE       (1 << 2)
#define F6_ROLL_SAFE        (1 << 3)
#define F6_ESC_LIMIT        (1 << 4)
#define F6_ESC_PATTERN      (1 << 5)
#define F6_MASK             0x3F

typedef struct {
    uint8_t  state;
    uint8_t  weight;
    uint8_t  target_c;
    uint8_t  prev_state;
    uint32_t pid;
    uint32_t flags;
} schema_instance_t;

void    schema_init(schema_instance_t *inst, uint32_t pid, uint8_t target);
uint8_t schema_eval(schema_instance_t *inst, uint32_t flags);
void    schema_boot(void);

#endif
