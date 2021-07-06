//===--------------------------------------------------------------------------------------------===
// warp_internal.h - Internal data structures supporting Warp
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#pragma once
#include <warp/warp.h>
#include "chunk.h"

struct warp_vm_t {
    chunk_t *chunk;
    uint8_t *ip;
    
    warp_value_t stack[WARP_STACK_MAX];
    warp_value_t *sp;
    
    size_t allocated;
    allocator_t allocator;
};

warp_result_t warp_run(warp_vm_t *vm, chunk_t *chunk);
