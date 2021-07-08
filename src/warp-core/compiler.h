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


bool compile(warp_vm_t *vm, chunk_t *chunk, const char *src, size_t length);
