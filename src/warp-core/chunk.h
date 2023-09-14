//===--------------------------------------------------------------------------------------------===
// chunk.h - Code chunk + debug symbols API
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#ifndef _CHUNK_H_
#define _CHUNK_H_
#include <warp/warp.h>
#include "buffers.h"

typedef struct chunk_t {
    int capacity;
    int count;
    
    int *lines;
    uint8_t *code;
    
    val_buf_t constants;
} chunk_t;

void chunk_init(warp_vm_t *vm, chunk_t *chunk);
void chunk_fini(warp_vm_t *vm, chunk_t *chunk);
void chunk_write(warp_vm_t *vm, chunk_t *chunk, uint8_t byte, int line);

int chunk_add_const(warp_vm_t *vm, chunk_t *chunk, warp_value_t value);

#endif /* _CHUNK_H_ */
