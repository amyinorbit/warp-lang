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
#include <warp/obj.h>
#include "chunk.h"

typedef void *(*allocator_t)(void *, size_t);

struct warp_vm_t {
    chunk_t         *chunk;
    uint8_t         *ip;
    
    warp_obj_t      *objects;
    
    warp_value_t    stack[WARP_STACK_MAX];
    warp_value_t    *sp;
    
    size_t          allocated;
    void            *(*allocator)(void *, size_t);
};
