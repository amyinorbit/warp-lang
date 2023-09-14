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

#define MAX_FRAMES  (64)
#define STACK_MAX   (MAX_FRAMES * UINT8_MAX)

typedef struct {
    warp_fn_t       *fn;
    uint8_t         *ip;
    warp_value_t    *slots;
} call_frame_t;

struct warp_vm_t {
    call_frame_t    frames[MAX_FRAMES];
    warp_int_t      frame_count;
    
    uint8_t         *ip;
    
    warp_obj_t      *objects;
    warp_map_t      *strings;
    warp_map_t      *globals;
    
    warp_value_t    stack[WARP_STACK_MAX];
    warp_value_t    *sp;
    
    size_t          allocated;
    void            *(*allocator)(void *, size_t);
};
