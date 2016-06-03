/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2011 Fergus Noble <fergusnoble@gmail.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBOPENCM3_MEMORYMAP_COMMON_H
#define LIBOPENCM3_MEMORYMAP_COMMON_H

#if defined(STM32F0)
#	include "./f0/memorymap.h"
#elif defined(STM32F1)
#	include "./f1/memorymap.h"
#elif defined(STM32F2)
#	include "./f2/memorymap.h"
#elif defined(STM32F3)
#	include "./f3/memorymap.h"
#elif defined(STM32F4)
#	include "./f4/memorymap.h"
#elif defined(STM32F7)
#	include "./f7/memorymap.h"
#elif defined(STM32L0)
#	include "./l0/memorymap.h"
#elif defined(STM32L1)
#	include "./l1/memorymap.h"
#elif defined(STM32L4)
#	include "./l4/memorymap.h"
#else
#	error "stm32 family not defined."
#endif

#endif /* LIBOPENCM3_MEMORYMAP_COMMON_H */
