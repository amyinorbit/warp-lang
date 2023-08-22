//===--------------------------------------------------------------------------------------------===
// common.h - Common includes and functions for the Warp language
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
    
#define PATH_SEP '/'
#define UNUSED(expr) (void)(expr)

#ifndef WARP_ENABLE_DEBUG
    #define WARP_ENABLE_DEBUG 1
#endif
    
#define DEBUG_PRINT_CODE 0
#define DEBUG_TRACE_EXEC 0
#define WARP_USE_NAN
    
#ifndef NDEBUG
    #define ASSERT(expr) UNUSED(expr)
    
    #if defined( _MSC_VER )
        #define UNREACHABLE() __assume(0)
    #elif (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5))
        #define UNREACHABLE() __builtin_unreachable()
    #else
        #define UNREACHABLE()
    #endif
#else
    #define ASSERT(expr) CHECK(expr)
    #define UNREACHABLE()                                                                          \
        do {                                                                                       \
            warp_log(__FILE__, __LINE__, "in %s(): this code should not be reached", __func__);    \
            abort();                                                                               \
        } while(0) 
#endif

#define CHECK(expr) \
    do {                                                                                           \
        if(!(expr)) {                                                                              \
            warp_log(__FILE__, __LINE__, "assertion failed: %s", #expr);                           \
            abort();                                                                               \
        }                                                                                          \
    } while(0)


void warp_log_init(void (*print_fn)(const char*));
void warp_log(const char *filename, int line, const char *fmt, ...);


typedef struct warp_vm_t warp_vm_t;
typedef struct warp_obj_t warp_obj_t;

#ifdef __cplusplus
} // extern "C"
#endif

