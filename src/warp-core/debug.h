//===--------------------------------------------------------------------------------------------===
// debug.h - debugging tools for Warp's VM
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#include "chunk.h"
#include "warp_internal.h"
#include <stdio.h>

void print_value(warp_value_t value, FILE *out);
void disassemble_chunk(chunk_t *chunk, const char *name, FILE *out);
int disassemble_instr(chunk_t *chunk, int offset, FILE *out);
