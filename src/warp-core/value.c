//===--------------------------------------------------------------------------------------------===
// value.c
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#include "value_impl.h"
#include "types/obj_impl.h"

static inline warp_value_kind_t value_kind(warp_value_t val) {
#ifdef WARP_USE_NAN
    if(WARP_IS_NUM(val)) return VAL_NUM;
    if(WARP_IS_OBJ(val)) return VAL_OBJ;
    switch(val & ~(QNAN)) {
        case TAG_NIL: return VAL_NIL;
        case TAG_TRUE: // fallthrough
        case TAG_FALSE: return VAL_BOOL;
    }
    UNREACHABLE();
    return VAL_NIL;
#else
    return val.kind;
#endif
}

bool value_is_falsey(warp_value_t a) {
    return WARP_IS_NIL(a) || (WARP_IS_BOOL(a) && !WARP_AS_BOOL(a));
}

bool value_equals(warp_value_t a, warp_value_t b) {
#ifdef WARP_USE_NAN
    UNUSED(value_kind);
    if(WARP_IS_OBJ(a) && WARP_IS_OBJ(b)) {
        return obj_equals(WARP_AS_OBJ(a), WARP_AS_OBJ(b));
    } else {
        return a == b;
    }
#else
    if(value_kind(a) != value_kind(b)) return false;
    switch(value_kind(a)) {
    case VAL_NIL: return true;
    case VAL_BOOL: return WARP_AS_BOOL(a) == WARP_AS_BOOL(b);
    case VAL_NUM: return WARP_AS_NUM(a) == WARP_AS_NUM(b);
    case VAL_OBJ: return return WARP_AS_OBJ(a) == WARP_AS_OBJ(b);
    //obj_equals(WARP_AS_OBJ(a), WARP_AS_OBJ(b));
    default: break;
    }
    UNREACHABLE();
    return false;
#endif
}
