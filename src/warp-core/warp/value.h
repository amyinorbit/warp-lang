//===--------------------------------------------------------------------------------------------===
// value.h - Warp's value system
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
    
#ifdef WARP_USE_NAN
    
#error "not implemented"
    
    
#else
typedef enum {
    VAL_NIL,
    VAL_BOOL,
    VAL_NUMBER,
} warp_value_kind_t;
    
typedef struct warp_value_s warp_value_t;

struct warp_value_s {
    warp_value_kind_t kind;
    union {
        double number;
        bool boolean;
    } as;
};

#define WARP_IS_NIL(val) ((val).kind == VAL_NIL)
#define WARP_IS_BOOL(val) ((val).kind == VAL_BOOL)
#define WARP_IS_NUMBER(val) ((val).kind == VAL_NUMBER)

#define WARP_AS_BOOL(val) ((val).as.boolean)
#define WARP_AS_NUMBER(val) ((val).as.number)

#define WARP_NIL_VAL ((warp_value_t){VAL_NIL, {.number=0}})
#define WARP_BOOL_VAL(value) ((warp_value_t){VAL_BOOL, {.boolean=!!(value)}})
#define WARP_NUMBER_VAL(value) ((warp_value_t){VAL_NUMBER, {.number=(value)}})
    
#endif


#ifdef __cplusplus
} // extern "C"
#endif

