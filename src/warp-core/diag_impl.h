//===--------------------------------------------------------------------------------------------===
// diag.h - Diagnosis 
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#pragma once
#include <warp/diag.h>
#include <stdarg.h>
#include "parser.h"

void emit_diag(
    const src_t *src,
    warp_diag_level_t level,
    const token_t *token,
    const char *fmt,
    ...
);
    
void emit_diag_loc(
    const src_t *src,
    warp_diag_level_t level,
    int line, const char *loc, int length,
    const char *fmt,
    ...
);

void emit_diag_varg(
    const src_t *src,
    warp_diag_level_t level,
    const token_t *token,
    const char *fmt,
    va_list args
);
    
void emit_diag_loc_varg(
    const src_t *src,
    warp_diag_level_t level,
    int line, const char *loc, int length,
    const char *fmt,
    va_list args
);

void emit_diag_line_varg(
    const src_t *src,
    warp_diag_level_t level,
    int line,
    const char *fmt,
    va_list args
);
