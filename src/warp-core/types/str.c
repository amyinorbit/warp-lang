/*===--------------------------------------------------------------------------------------------===
 * str.c
 *
 * Created by Amy Parent <amy@amyparent.com>
 * Copyright (c) 2023 Amy Parent. All rights reserved
 *
 * Licensed under the MIT License
 *===--------------------------------------------------------------------------------------------===
*/
#include "obj_impl.h"
#include <string.h>
#include <stdio.h>


static uint32_t str_hash(const char *chars, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)chars[i];
        hash *= 16777619;
    }
    return hash;
}

warp_str_t *alloc_str(warp_vm_t *vm, int length) {
    warp_str_t *str = ALLOCATE_SARRAY(vm, warp_str_t, length + 1);
    init_obj(vm, (warp_obj_t *)str, WARP_OBJ_STR);
    str->length = length;
    return str;
}

warp_str_t *warp_copy_c_str(warp_vm_t *vm, const char *c_str, int length) {
    warp_str_t *str = alloc_str(vm, length);
    memcpy(str->data, c_str, length);
    str->data[length] = '\0';
    str->length = length;
    str->hash = str_hash(c_str, length);
    return str;
}

warp_str_t *warp_concat_str(warp_vm_t *vm, const warp_str_t *a, const warp_str_t *b) {
    warp_str_t *str = alloc_str(vm, a->length + b->length);
    memcpy(str->data, a->data, a->length);
    memcpy(str->data + a->length, b->data, b->length);
    str->data[str->length] = '\0';
    str->hash = str_hash(str->data, str->length);
    return str;
}

int warp_str_get_length(const warp_str_t *str) {
    return str->length;
}

const char *warp_str_get_c(const warp_str_t *str) {
    return str->data;
}


