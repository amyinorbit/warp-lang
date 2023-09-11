//===--------------------------------------------------------------------------------------------===
// chunk.c - One portion of Warp VM bytecode
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#include "chunk.h"
#include "value_impl.h"
#include <warp/instr.h>
#include "warp_internal.h"

void chunk_init(warp_vm_t *vm, chunk_t *chunk) {
    ASSERT(chunk);
    UNUSED(vm); // We could remove vm from the args, but homogeneity? We'll see
    
    chunk->lines = NULL;
    chunk->code = NULL;
    chunk->count = 0;
    chunk->capacity = 0;
    
    val_buf_init(&chunk->constants);
}

void chunk_fini(warp_vm_t *vm, chunk_t *chunk) {
    ASSERT(vm);
    ASSERT(chunk);
    
    FREE_ARRAY(vm, chunk->code, uint8_t, chunk->capacity);
    FREE_ARRAY(vm, chunk->lines, int, chunk->capacity);
    val_buf_fini(vm, &chunk->constants);
    chunk_init(vm, chunk);
}

void chunk_write(warp_vm_t *vm, chunk_t *chunk, uint8_t byte, int line) {
    ASSERT(vm);
    ASSERT(chunk);
    
    if(chunk->capacity < chunk->count + 1) {
        size_t old_cap = chunk->capacity;
        size_t new_cap = GROW_CAPACITY(chunk->capacity);
        chunk->code = GROW_ARRAY(vm, chunk->code, uint8_t, old_cap, new_cap);
        chunk->lines = GROW_ARRAY(vm, chunk->lines, int, old_cap, new_cap);
        chunk->capacity = new_cap;
    }
    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->count += 1;
}

int chunk_add_const(warp_vm_t *vm, chunk_t *chunk, warp_value_t value) {
    ASSERT(vm);
    ASSERT(chunk);
    
    for(int i = 0; i < chunk->constants.count; ++i) {
        if(value_equals(value, chunk->constants.data[i])) return i;
    }
    
    val_buf_write(vm, &chunk->constants, value);
    ASSERT(chunk->constants.count < UINT16_MAX+1);
    return chunk->constants.count - 1;
}


