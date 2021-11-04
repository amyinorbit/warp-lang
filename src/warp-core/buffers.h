//===--------------------------------------------------------------------------------------------===
// buffers.h - Macro-backed variable length buffers.
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#pragma once
#include <warp/warp.h>
#include "memory.h"

#define DECLARE_BUFFER(name, T)                                                                    \
    typedef struct {                                                                               \
        T* data;                                                                                   \
        int32_t count;                                                                             \
        int32_t capacity;                                                                          \
    } name##_buf_t;                                                                                \
    void name##_buf_init(name##_buf_t* buffer);                                                    \
    void name##_buf_fini(warp_vm_t *vm, name##_buf_t* buffer);                                     \
    void name##_buf_fill(warp_vm_t *vm, name##_buf_t* buffer, T data,  int32_t count);             \
    void name##_buf_write(warp_vm_t *vm, name##_buf_t* buffer, T data);                            \
    T *name##_buf_take(name##_buf_t *buffer)

// This should be used once for each T instantiation, somewhere in a .c file.
#define DEFINE_BUFFER(name, T)                                                                     \
    void name##_buf_init(name##_buf_t* buffer) {                                                   \
        buffer->data = NULL;                                                                       \
        buffer->capacity = 0;                                                                      \
        buffer->count = 0;                                                                         \
    }                                                                                              \
                                                                                                   \
    void name##_buf_fini(warp_vm_t* vm, name##_buf_t* buffer) {                                    \
        FREE_ARRAY(vm, buffer->data, T, buffer->capacity * sizeof(T));                             \
        name##_buf_init(buffer);                                                                   \
    }                                                                                              \
                                                                                                   \
    void name##_buf_fill(warp_vm_t* vm, name##_buf_t* buffer, T data, int count) {                 \
        if(buffer->capacity < buffer->count + count) {                                             \
            int old_cap = buffer->capacity;                                                        \
            while(buffer->capacity < buffer->count + count)                                        \
                buffer->capacity = GROW_CAPACITY(buffer->capacity);                                \
                buffer->data = GROW_ARRAY(vm, buffer->data, T, old_cap, buffer->capacity);         \
        }                                                                                          \
                                                                                                   \
        for(int i = 0; i < count; i++) {                                                           \
            buffer->data[buffer->count++] = data;                                                  \
        }                                                                                          \
    }                                                                                              \
                                                                                                   \
    void name##_buf_write(warp_vm_t* vm, name##_buf_t* buffer, T data) {                           \
        name##_buf_fill(vm, buffer, data, 1);                                                      \
    }                                                                                              \
                                                                                                   \
                                                                                                   \
    T *name##_buf_take(name##_buf_t *buffer) {                                                     \
        T *data = buffer->data;                                                                    \
        name##_buf_init(buffer);                                                                   \
        return data;                                                                               \
    }                                                                                              \

DECLARE_BUFFER(str, char);
DECLARE_BUFFER(u8, uint8_t);
DECLARE_BUFFER(i32, int32_t);
DECLARE_BUFFER(f32, float);
DECLARE_BUFFER(val, warp_value_t);

