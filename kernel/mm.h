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

#ifndef MM_H
#define MM_H

typedef unsigned char  uint8_t;
typedef unsigned int   uint32_t;

void     mm_init(uint32_t heap_start, uint32_t heap_end);
void    *kmalloc(uint32_t size);
void     kfree(void *ptr);
uint32_t mm_used(void);
uint32_t mm_total(void);

#endif
