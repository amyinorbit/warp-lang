//===--------------------------------------------------------------------------------------------===
// memory.h - Memory macros
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#pragma once
#include <warp/warp.h>
#include <stdlib.h>

#define GROW_CAPACITY(cap) (((cap) < 8) ? 8 : (cap) * 2)

#define GROW_ARRAY(vm, array, T, old_count, new_count) \
    warp_alloc((vm), (array), old_count * sizeof(T), new_count * sizeof(T))

#define FREE_ARRAY(vm, array, T, old_count) \
    warp_alloc((vm), (array), old_count * sizeof(T), 0)

#define ALLOCATE_SARRAY(vm, T, size) \
    (T *)warp_alloc((vm), NULL, 0, sizeof(T) + (size))

#define ALLOCATE(vm, T) \
    warp_alloc((vm), NULL, 0, sizeof(T))

void *default_allocator(void *ptr, size_t req);
void *warp_alloc(warp_vm_t *vm, void *ptr, size_t old_size, size_t new_size);
