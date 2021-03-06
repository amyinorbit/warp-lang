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
#include "scanner.h"

void emit_diag_varg(
    const src_t *src,
    warp_diag_level_t level,
    const token_t *token,
    const char *fmt,
    va_list args
);
