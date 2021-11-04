//===--------------------------------------------------------------------------------------------===
// diag.h - Diagnostics API for the Warp programming language
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#pragma once
#include <warp/common.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WARP_DIAG_INFO,
    WARP_DIAG_WARN,
    WARP_DIAG_ERROR,
} warp_diag_level_t;

typedef struct warp_src_span_s {
    int line;
    int column;
    int length;
} warp_src_span_t;

typedef struct warp_diag_s {
    warp_diag_level_t level;
    const char *message;
    
    warp_src_span_t span;
    
    const char *fname;
    const char *line_start;
    const char *line_end;
} warp_diag_t;

void warp_print_diag(const warp_diag_t *diag, void *user_info);

#ifdef __cplusplus
} // extern "C"
#endif

