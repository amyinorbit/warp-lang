//===--------------------------------------------------------------------------------------------===
// chunk.c - One portion of Warp VM bytecode
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#include "chunk.h"
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
    chunk_init(vm, chunk);
    val_buf_fini(vm, &chunk->constants);
}

void chunk_emit_8(warp_vm_t *vm, chunk_t *chunk, uint8_t byte, int line) {
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

void chunk_emit_16(warp_vm_t *vm, chunk_t *chunk, uint16_t bytes, int line) {
    ASSERT(vm);
    ASSERT(chunk);
    
    chunk_emit_8(vm, chunk, bytes & 0x00ff, line);
    chunk_emit_8(vm, chunk, bytes >> 8, line);
}

void chunk_emit_const(warp_vm_t *vm, chunk_t *chunk, warp_value_t value, int line) {
    ASSERT(vm);
    ASSERT(chunk);
    int idx = chunk_add_const(vm, chunk, value);
    if(idx < 255) { // TODO: FLIP THAT LOGIC ONCE TESTED
        chunk_emit_8(vm, chunk, OP_LCONST, line);
        chunk_emit_16(vm, chunk, (uint16_t)idx, line);
    } else {
        chunk_emit_8(vm, chunk, OP_CONST, line);
        chunk_emit_8(vm, chunk, (uint8_t)idx, line);
    }
}

int chunk_add_const(warp_vm_t *vm, chunk_t *chunk, warp_value_t value) {
    ASSERT(vm);
    ASSERT(chunk);
    
    val_buf_write(vm, &chunk->constants, value);
    ASSERT(chunk->constants.count < 255);
    return chunk->constants.count - 1;
}


