//===--------------------------------------------------------------------------------------------===
// value.c
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#include "value.h"

static inline warp_value_kind_t value_kind(warp_value_t val) {
#ifdef WARP_USE_NAN
#error "not implemented"
#else
    return val.kind;
#endif
}

bool value_is_falsey(warp_value_t a) {
    return WARP_IS_NIL(a) || (WARP_IS_BOOL(a) && !WARP_AS_BOOL(a));
}

bool value_equals(warp_value_t a, warp_value_t b) {
    if(value_kind(a) != value_kind(b)) return false;
    switch(value_kind(a)) {
    case VAL_NIL: return true;
    case VAL_BOOL: return WARP_AS_BOOL(a) == WARP_AS_BOOL(b);
    case VAL_NUMBER: return WARP_AS_NUMBER(a) == WARP_AS_NUMBER(b);
    default: break;
    }
    UNREACHABLE();
}
