/*===--------------------------------------------------------------------------------------------===
 * debug.h
 *
 * Created by Amy Parent <amy@amyparent.com>
 * Copyright (c) 2023 Amy Parent
 *
 * Licensed under the MIT License
 *===--------------------------------------------------------------------------------------------===
*/
#ifndef _WARP_DEBUG_H_
#define _WARP_DEBUG_H_

#include <warp/value.h>
#include "chunk.h"
#include <stdio.h>

void print_value(warp_value_t value, FILE *out);
void disassemble_chunk(chunk_t *chunk, const char *name, FILE *out);
int disassemble_instr(chunk_t *chunk, int offset, FILE *out);

#endif /* ifndef _WARP_DEBUG_H_ */
