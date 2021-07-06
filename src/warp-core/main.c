//===--------------------------------------------------------------------------------------------===
// main.c - Test executable for Warp
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#include <stdio.h>
#include "chunk.h"
#include "debug.h"
#include <warp/warp.h>
#include <warp/instr.h>

int main(int argc, const char **argv) {
    UNUSED(argc);
    UNUSED(argv);
    
    warp_vm_t *vm = warp_vm_new(&(warp_cfg_t){.allocator = NULL});
    chunk_t chunk;
    
    chunk_init(vm, &chunk);
    
    chunk_emit_const(vm, &chunk, 123.5, 53);
    chunk_emit_8(vm, &chunk, OP_NEG, 53);
    
    chunk_emit_const(vm, &chunk, 456, 54);
    chunk_emit_8(vm, &chunk, OP_ADD, 54);
    chunk_emit_8(vm, &chunk, OP_RETURN, 55);
    
    // disassemble_chunk(&chunk, "test chunk", stdout);
    warp_run(vm, &chunk);
    chunk_fini(vm, &chunk);
    
    warp_vm_destroy(vm);
}
