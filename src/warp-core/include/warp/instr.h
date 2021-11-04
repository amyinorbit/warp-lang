//===--------------------------------------------------------------------------------------------===
// instr.h - Warp's virtual instruction set
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define WARP_OP(code, _, __) OP_##code,
typedef enum {
#include "instr.def"
} warp_opcode_t;
#undef WARP_OP

#ifdef __cplusplus
} // extern "C"
#endif

