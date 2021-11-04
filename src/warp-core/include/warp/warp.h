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
#include <warp/diag.h>
#include <warp/value.h>

#ifdef __cplusplus
extern "C" {
#endif
    
#ifndef WARP_STACK_MAX
#define WARP_STACK_MAX (256)
#endif

typedef enum {
    WARP_OK,
    WARP_COMPILE_ERROR,
    WARP_RUNTIME_ERROR,
} warp_result_t;

typedef struct warp_cfg_t {
    void *(*allocator)(void *, size_t);
    struct {
        void (*compile_diag)(const warp_diag_t *, void *);
        void (*runtime_diag)(const char *message, void *);
        void *user_info;
    } diagnostics;
} warp_cfg_t;

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

warp_result_t warp_run(warp_vm_t *vm);
warp_result_t warp_interpret(warp_vm_t *vm, const char *source, size_t length);

#ifdef __cplusplus
} // extern "C"
#endif

