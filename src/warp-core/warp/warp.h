//===--------------------------------------------------------------------------------------------===
// warp.h - The Warp programming language!
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#pragma once
#include <warp/common.h>
#include <warp/value.h>

#ifdef __cplusplus
extern "C" {
#endif
    
#define WARP_STACK_MAX (256)

typedef struct warp_vm_t warp_vm_t;
typedef void *(*allocator_t)(void *, size_t);

typedef struct warp_cfg_t {
    allocator_t allocator;
} warp_cfg_t;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} warp_result_t;

/**
 * Creates and resets a new Warp virtual machine.
 *
 * @param cfg The configuration used when creating the VM.
 * @return The newly created virtual machine.
 */
warp_vm_t *warp_vm_new(const warp_cfg_t *cfg);

/*!
 * Destroys a Warp virtual machine and all its associated context.
 *
 * @param vm The Warp VM to destroy.
 */
void warp_vm_destroy(warp_vm_t *vm);

warp_result_t warp_vm_run(warp_vm_t *vm, const char *source);


#ifdef __cplusplus
} // extern "C"
#endif

