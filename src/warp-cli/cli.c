//===--------------------------------------------------------------------------------------------===
// cli.c - The Warp language command-line interface.
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#include <warp/warp.h>
#include <stdio.h>

static void repl() {
    
    warp_vm_t *vm = warp_vm_new(&(warp_cfg_t){.allocator = NULL});
    
    char line[1024];
    for(;;) {
        printf("> ");
        
        if(!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }
        
        warp_vm_run(vm, line);
    }
    
    warp_vm_destroy(vm);
}

static void run_file(const char *path) {
    warp_vm_t *vm = warp_vm_new(&(warp_cfg_t){.allocator = NULL});
    warp_vm_run(vm, "print 1 + 3");
    warp_vm_destroy(vm);
}

int main(int argc, const char **argv) {
    if(argc == 1) {
        repl();
    } else if(argc == 2) {
        run_file(argv[1]);
    } else {
        fprintf(stderr, "Usage: warp [path]\n");
        exit(64);
    }
}

