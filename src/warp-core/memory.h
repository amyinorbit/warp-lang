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
    vm_alloc((vm), (array), old_count * sizeof(T), new_count * sizeof(T))

#define FREE_ARRAY(vm, array, T, old_count) \
    vm_alloc((vm), (array), old_count * sizeof(T), 0)

void *default_allocator(void *ptr, size_t req);
void *vm_alloc(warp_vm_t *vm, void *ptr, size_t old_size, size_t new_size);
