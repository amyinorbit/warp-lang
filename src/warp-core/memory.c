//===--------------------------------------------------------------------------------------------===
// memory.c
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#include "memory.h"
#include "warp_internal.h"

void *default_allocator(void *ptr, size_t req) {
    if(req == 0) {
        free(ptr);
        return NULL;
    }
    
    void *result = realloc(ptr, req);
    CHECK(result);
    return result;
}

void *warp_alloc(warp_vm_t *vm, void *ptr, size_t old_size, size_t new_size) {
    ASSERT(vm);
    
    vm->allocated += (new_size - old_size);
    return vm->allocator(ptr, new_size);
}

