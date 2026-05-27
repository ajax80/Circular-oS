/*
 * Circular OS
 * Copyright (C) 2026 Jonathan Eugene Ayers <ayersjon80@gmail.com>
 *
 * This file is part of Circular OS.
 *
 * Circular OS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Circular OS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public
 * License along with Circular OS. If not, see
 * <https://www.gnu.org/licenses/>.
 *
 * For commercial licensing, see COMMERCIAL_LICENSE in this repository.
 */

#include "mm.h"
#include "serial.h"

#define ALIGN4(x) (((x) + 3) & ~3u)

typedef struct block {
    uint32_t       size;
    uint8_t        free;
    struct block  *next;
} block_t;

static block_t *heap_head = 0;
static uint32_t heap_brk  = 0;
static uint32_t heap_end  = 0;

void mm_init(uint32_t start, uint32_t end) {
    heap_brk  = ALIGN4(start);
    heap_end  = end;
    heap_head = 0;
}

static block_t *find_free(uint32_t size) {
    block_t *b = heap_head;
    while (b) {
        if (b->free && b->size >= size) return b;
        b = b->next;
    }
    return 0;
}

void *kmalloc(uint32_t size) {
    if (!size) return 0;
    size = ALIGN4(size);

    block_t *b = find_free(size);
    if (b) {
        b->free = 0;
        return (void *)((uint8_t *)b + sizeof(block_t));
    }

    uint32_t need = sizeof(block_t) + size;
    if (heap_brk + need > heap_end) return 0;

    b = (block_t *)heap_brk;
    heap_brk += need;
    b->size = size;
    b->free = 0;
    b->next = heap_head;
    heap_head = b;
    return (void *)((uint8_t *)b + sizeof(block_t));
}

void kfree(void *ptr) {
    if (!ptr) return;
    block_t *b = (block_t *)((uint8_t *)ptr - sizeof(block_t));
    b->free = 1;
}

uint32_t mm_used(void) {
    uint32_t used = 0;
    block_t *b = heap_head;
    while (b) {
        if (!b->free) used += b->size;
        b = b->next;
    }
    return used;
}

uint32_t mm_total(void) {
    return heap_end - (uint32_t)heap_head;
}
