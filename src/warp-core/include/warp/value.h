//===--------------------------------------------------------------------------------------------===
// value.h - Warp's value system
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#ifndef _WARP_VALUE_H_
#define _WARP_VALUE_H_
#include <warp/common.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VAL_NIL,
    VAL_BOOL,
    VAL_NUM,
    VAL_OBJ,
} warp_value_kind_t;

typedef struct warp_obj_t   warp_obj_t;
typedef struct warp_str_t   warp_str_t;
typedef struct warp_list_t  warp_list_t;
typedef struct warp_map_t   warp_map_t;
typedef struct warp_fn_t    warp_fn_t;
typedef struct warp_native_t warp_native_t;

typedef int32_t      warp_int_t;
typedef uint32_t     warp_uint_t;
    
#ifdef WARP_USE_NAN
typedef uint64_t warp_value_t;

static inline double value2double(warp_value_t value) {
    union { uint64_t bits; double num; } data;
    data.bits = value;
    return data.num;
}

static inline warp_value_t double2value(double value) {
    union { uint64_t bits; double num; } data;
    data.num = value;
    return data.bits;
}
    
#define SIGN_BIT ((uint64_t)0x8000000000000000)
#define QNAN ((uint64_t)0x7ffc000000000000)

#define TAG_NIL 1
#define TAG_FALSE 2
#define TAG_TRUE 3
    
#define WARP_NIL_VAL        ((warp_value_t)(QNAN | TAG_NIL))
#define WARP_FALSE_VAL      ((warp_value_t)(QNAN | TAG_FALSE))
#define WARP_TRUE_VAL       ((warp_value_t)(QNAN | TAG_TRUE))
#define WARP_BOOL_VAL(val)  ((val) ? WARP_TRUE_VAL : WARP_FALSE_VAL)
#define WARP_NUM_VAL(val)   (double2value(val))
#define WARP_OBJ_VAL(val)   (SIGN_BIT | QNAN | (uint64_t)((uintptr_t)(val)))
    
#define WARP_AS_BOOL(val)   ((val) == WARP_TRUE_VAL)
#define WARP_AS_NUM(val)    (value2double(val))
#define WARP_AS_OBJ(val)    ((warp_obj_t *)(uintptr_t)((val) & ~(SIGN_BIT | QNAN)))

#define WARP_IS_NIL(val)    ((val) == WARP_NIL_VAL)
#define WARP_IS_BOOL(val)   (((val) | 1) == WARP_TRUE_VAL)
#define WARP_IS_NUM(val)    (((val) & QNAN) != QNAN)
#define WARP_IS_OBJ(val)    (((val) & (SIGN_BIT | QNAN)) == (SIGN_BIT | QNAN))
    
#else
    
typedef struct warp_value_t warp_value_t;

struct warp_value_t {
    warp_value_kind_t   kind;
    union {
        warp_obj_t      *obj;
        double          num;
        bool            boolean;
    } as;
};

#define WARP_IS_NIL(val)    ((val).kind == VAL_NIL)
#define WARP_IS_BOOL(val)   ((val).kind == VAL_BOOL)
#define WARP_IS_NUM(val)    ((val).kind == VAL_NUM)
#define WARP_IS_OBJ(val)    ((val).kind == VAL_OBJ)

#define WARP_AS_BOOL(val)   ((val).as.boolean)
#define WARP_AS_NUM(val)    ((val).as.num)
#define WARP_AS_OBJ(val)    ((val).as.obj)

#define WARP_NIL_VAL        ((warp_value_t){VAL_NIL, {.num=0}})
#define WARP_BOOL_VAL(val)  ((warp_value_t){VAL_BOOL, {.boolean=!!(val)}})
#define WARP_NUM_VAL(val)   ((warp_value_t){VAL_NUM, {.num=(val)}})
#define WARP_OBJ_VAL(val)   ((warp_value_t){VAL_NUM, {.obj=(warp_obj_t *)(val)}})
    
#endif

typedef void (*warp_native_f)(warp_vm_t *vm, warp_value_t *args);

void warp_print_value(warp_value_t value, FILE *out);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* _WARP_VALUE_H_ */
