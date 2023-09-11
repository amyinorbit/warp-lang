//===--------------------------------------------------------------------------------------------===
// debug.c - Disassembler and debug routines
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#include "debug.h"
#include <warp/instr.h>
#include <warp/obj.h>

typedef struct {
    const char *name;
    int operand;
} instr_data_t;

#define WARP_OP(name, op, __) {#name, op},
static const instr_data_t instr_data[] = {
#include <warp/instr.def>
};
#undef WARP_OP

static const size_t instr_count = sizeof(instr_data)/sizeof(instr_data_t);

static void print_obj(warp_value_t val, FILE *out) {
    
    switch(WARP_OBJ_KIND(val)) {
    case WARP_OBJ_STR:
        fprintf(out, "%s", WARP_AS_CSTR(val));
        break;
    case WARP_OBJ_MAP:
        fprintf(out, "<map>");
        break;
    }
}

void print_value(warp_value_t value, FILE *out) {
    ASSERT(out);
    if(WARP_IS_NUM(value)) {
        fprintf(out, "%g", WARP_AS_NUM(value));
    } else if(WARP_IS_BOOL(value)) {
        fprintf(out, "%s", WARP_AS_BOOL(value) ? "true" : "false");
    } else if(WARP_IS_NIL(value)) {
        fprintf(out, "<nil>");
    } else if(WARP_IS_OBJ(value)) {
        print_obj(value, out);
    }
}

void disassemble_chunk(chunk_t *chunk, const char *name, FILE *out) {
    ASSERT(chunk);
    ASSERT(out);
    fprintf(out, "** %s **\n", name);
    
    for(int offset = 0; offset < chunk->count;) {
        offset = disassemble_instr(chunk, offset, out);
    }
}

int disassemble_instr(chunk_t *chunk, int offset, FILE *out) {
    ASSERT(chunk);
    ASSERT(out);
    ASSERT(offset < chunk->count);
    
    fprintf(out, "%04x: ", offset);
    if(offset > 0 && chunk->lines[offset-1] == chunk->lines[offset]) {
        fprintf(out, "   | ");
    } else {
        fprintf(out, "%4d ", chunk->lines[offset]);
    }
    
    uint8_t op = chunk->code[offset];
    if(op >= instr_count) {
        fprintf(out, "<illegal instr: %02x>\n", op);
        return offset;
    }
    
    switch(instr_data[op].operand) {
    case 0:
        fprintf(out, "%-16s\n", instr_data[op].name);
        break;
    case 1:
        fprintf(out, "%-16s %02hhx", instr_data[op].name, chunk->code[offset+1]);
        if(op == OP_CONST || op == OP_DEF_GLOB || op == OP_GET_GLOB || op == OP_SET_GLOB) {
            uint8_t value_idx = chunk->code[offset+1];
            fprintf(out, "  (");
            print_value(chunk->constants.data[value_idx], out);
            fprintf(out, ")\n");
        } else {
            fprintf(out, "\n");
        }
        break;
    case 2:
        fprintf(out, "%-16s %02hhx %02hhx\n", instr_data[op].name, chunk->code[offset+2], chunk->code[offset+1]);
        break;
    default:
        UNREACHABLE();
        break;
    }
    
    return offset + 1 + instr_data[op].operand;
}
