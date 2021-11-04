//===--------------------------------------------------------------------------------------------===
// string.h - Warp's string primitive
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#pragma once
#include <warp/value.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct warp_str_t warp_str_t;

warp_str_t *warp_copy_c_str(warp_vm_t *vm, const char *c_str);
warp_str_t *warp_take_c_str(warp_vm_t *vm, const char *c_str);
warp_str_t *warp_concat_str(warp_vm_t *vm, const warp_str_t *a, const warp_str_t *b);

size_t warp_str_get_length(const warp_str_t *str);
size_t warp_str_equals(const warp_str_t *a, const warp_str_t *b);
const char *warp_str_as_c(const warp_str_t *str);

#ifdef __cplusplus
} // extern "C"
#endif

