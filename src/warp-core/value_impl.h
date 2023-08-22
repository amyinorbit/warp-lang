//===--------------------------------------------------------------------------------------------===
// value.h - Private API for the Warp value system
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#include <warp/value.h>
// #include <warp/str.h>

// typedef struct warp_class_t warp_class_t;


// typedef enum {
//     OBJ_STRING,
//     OBJ_ARRAY,
//     OBJ_MAP
// } warp_obj_kind_t;
//
// struct warp_obj_t {
//     warp_class_t *isa;
//     warp_value_t fields[];
// };
//
// struct warp_instance_t {
//     int retain_count;
//     warp_class_t *isa;
//     warp_value_t fields[];
// };
//
// struct warp_class_t {
//     warp_str_t *name;
//     uint16_t field_count;
// };

bool value_is_falsey(warp_value_t a);
bool value_equals(warp_value_t a, warp_value_t b);
