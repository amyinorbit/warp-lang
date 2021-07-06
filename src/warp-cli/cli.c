//===--------------------------------------------------------------------------------------------===
// cli.c - The Warp language command-line interface.
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#include <warp/warp.h>
#include <term/line.h>
#include <term/colors.h>
#include <stdio.h>

static void repl_prompt(const char* PS) {
    term_set_fg(stdout, TERM_BLUE);
    printf("warp:%s> ", PS);
    term_set_fg(stdout, TERM_DEFAULT);
}

static const char* history_path() {
    const char* home = getenv("HOME");
    if(!home) return ".warp_history";

    static char path[4096];
    snprintf(path, 4096, "%s/.warp_history", home);
    return path;
}

static void repl() {
    
    warp_vm_t *vm = warp_vm_new(&(warp_cfg_t){.allocator = NULL});
    
    // Set up our fancy line editor
    UNUSED(repl_prompt);
    line_t *line_ed = line_new(&(line_functions_t){NULL});
    line_history_load(line_ed, history_path());
    line_set_prompt(line_ed, "repl");
    
    char *line = NULL;
    while((line = line_get(line_ed))) {
        warp_vm_run(vm, line);
        free(line);
    }
    
    line_history_write(line_ed, history_path());
    line_destroy(line_ed);
    
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

