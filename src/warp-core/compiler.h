//===--------------------------------------------------------------------------------------------===
// compiler.h
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#pragma once
#include <warp/warp.h>
#include "chunk.h"

warp_fn_t *compile(warp_vm_t *vm, const char *fname, const char *src, size_t length);
