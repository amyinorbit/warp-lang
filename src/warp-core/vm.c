//===--------------------------------------------------------------------------------------------===
// vm.c - Warp's VM innards
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#include <warp/instr.h>
#include <term/colors.h>
#include "warp_internal.h"
#include "compiler.h"
#include "memory.h"
#include "debug.h"
#include "value_impl.h"
#include "types/obj_impl.h"
#include <stdarg.h>

warp_vm_t *warp_vm_new(const warp_cfg_t *cfg) {
    ASSERT(cfg);
    allocator_t alloc = cfg->allocator ? cfg->allocator : default_allocator;
    warp_vm_t *vm = alloc(NULL, sizeof(warp_vm_t));
    CHECK(vm);
    
    vm->allocator = alloc;
    vm->objects = NULL;
    vm->strings = warp_map_new(vm);
    vm->globals = warp_map_new(vm);
    
    return vm;
}

void warp_vm_destroy(warp_vm_t *vm) {
    ASSERT(vm);
	
    warp_map_free(vm, vm->strings);
    warp_map_free(vm, vm->globals);
    vm->strings = NULL;
    vm->globals = NULL;
    for(warp_obj_t *obj = vm->objects; obj != NULL;) {
        warp_obj_t *next = obj->next;
        obj_destroy(vm, obj);
        obj = next;
    }
    vm->objects = NULL;
    vm->allocator(vm, 0);
}

static inline uint8_t read_8(warp_vm_t *vm) {
    return *vm->ip++;
}

// static inline uint16_t read_16(warp_vm_t *vm) {
//     return (uint16_t)(*vm->ip++) | ((uint16_t)(*vm->ip++) << 8);
// }

static inline warp_value_t read_constant(warp_vm_t *vm) {
    return vm->chunk->constants.data[read_8(vm)];
}

// static inline warp_value_t read_constant_long(warp_vm_t *vm) {
//     return vm->chunk->constants.data[read_16(vm)];
// }

static void reset_stack(warp_vm_t *vm) {
    vm->sp = vm->stack;
}

static inline void push(warp_vm_t *vm, warp_value_t value) {
    *vm->sp = value;
    vm->sp++;
}

static inline warp_value_t peek(warp_vm_t *vm, int offset) {
    return vm->sp[-1 - offset];
}

static inline warp_value_t pop(warp_vm_t *vm) {
    ASSERT(vm->sp > vm->stack);
    return *(--vm->sp);
}

static void runtime_error(warp_vm_t *vm, const char *fmt, ...) {
    // TODO: output to the diagnostics system, probably
    
    size_t instruction = vm->ip - vm->chunk->code;
    int line = vm->chunk->lines[instruction];

    fprintf(stderr, "runtime error on line %d: ", line);
    
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    reset_stack(vm);
    fputc('\n', stderr);
}

static void concatenate(warp_vm_t *vm) {
    warp_str_t *b = WARP_AS_STR(pop(vm));
    warp_str_t *a = WARP_AS_STR(pop(vm));
    
    push(vm, WARP_OBJ_VAL(warp_concat_str(vm, a, b)));
}

warp_result_t warp_run(warp_vm_t *vm) {
    ASSERT(vm);
    reset_stack(vm);
    
#define READ_8() (*vm->ip++)
#define READ_CONST() (vm->chunk->constants.data[READ_8()])
    
#define BINARY(T, op)                                                                              \
    do {                                                                                           \
        if(!WARP_IS_NUM(peek(vm, 0)) || !WARP_IS_NUM(peek(vm, 1))) {                               \
            runtime_error(vm, "Invalid operands to " #op " operator");                             \
            return WARP_RUNTIME_ERROR;                                                             \
        }                                                                                          \
        double b = WARP_AS_NUM(pop(vm));                                                           \
        double a = WARP_AS_NUM(pop(vm));                                                           \
        push(vm, WARP_##T##_VAL(a op b));                                                          \
    } while(0)
        
    
    for(;;) {
        uint8_t instr;
#if DEBUG_TRACE_EXEC == 1
        fprintf(stdout, "\n===\n");
        for(warp_value_t *v = vm->stack; v != vm->sp; ++v) {
            printf("[");
            print_value(*v, stdout);
            printf("]\n");
        }
        fprintf(stdout, "---\n");
        disassemble_instr(vm->chunk, (int)(vm->ip - vm->chunk->code), stdout);
        fprintf(stdout, "===\n");
#endif
        switch(instr = read_8(vm)) {            
        case OP_CONST:
            push(vm, read_constant(vm));
            break;
            
        case OP_DEF_GLOB: {
            warp_value_t name = read_constant(vm);
            warp_map_set(vm, vm->globals, name, peek(vm, 0));
            break;
        }
        
        case OP_GET_GLOB: {
            warp_value_t name = read_constant(vm);
            warp_value_t val = WARP_NIL_VAL;
            if(!warp_map_get(vm->globals, name, &val)) {
                runtime_error(vm, "undefined global variable '%s'", WARP_AS_CSTR(name));
                return WARP_RUNTIME_ERROR;
            }
            push(vm, val);
            break;
        }
        
        case OP_SET_GLOB: {
            warp_value_t name = read_constant(vm);
            if(!warp_map_set(vm, vm->globals, name, peek(vm, 0))) {
                warp_map_delete(vm->globals, name, NULL);
                runtime_error(vm, "undefined global variable '%s", WARP_AS_CSTR(name));
            }
            break;
        }
        
        case OP_GET_LOCAL: {
            uint8_t slot = READ_8();
            push(vm, vm->stack[slot]);
            break;
        }
        
        case OP_SET_LOCAL: {
            uint8_t slot = READ_8();
            vm->stack[slot] = peek(vm, 0);
            break;
        }
        
        case OP_DUP:
            push(vm, peek(vm, 0));
            break;
            
        case OP_POP:
            pop(vm);
            break;
            
        // because we are expression-oriented, the last result of a block is its value. So we
        // can't just POP our way out of all of our locals -- we need to save the top-of-stack
        // first.
        case OP_BLOCK: {
            uint8_t slots = READ_8();
            warp_value_t val = pop(vm);
            vm->sp -= slots;
            push(vm, val);
            break;
        }
            
        case OP_NIL:
            push(vm, WARP_NIL_VAL);
            break;
            
        case OP_TRUE:
            push(vm, WARP_BOOL_VAL(true));
            break;
            
        case OP_FALSE:
            push(vm, WARP_BOOL_VAL(false));
            break;
            
        case OP_NEG: {
            if(!WARP_IS_NUM(peek(vm, 0))) {
                // TODO: throw error
                runtime_error(vm, "invalid operands to `-'");
                return WARP_RUNTIME_ERROR;
            }
            double val = WARP_AS_NUM(pop(vm));
            push(vm, WARP_NUM_VAL(-val));
            break;
        }
        
        case OP_NOT: {
            warp_value_t val = pop(vm);
            push(vm, WARP_BOOL_VAL(value_is_falsey(val)));
            break;
        }
            
        case OP_ADD:
            if(WARP_IS_STR(peek(vm, 0)) && WARP_IS_STR(peek(vm, 1))) {
                concatenate(vm);
            } else if(WARP_IS_NUM(peek(vm, 0)) && WARP_IS_NUM(peek(vm, 1))) {
                double b = WARP_AS_NUM(pop(vm));
                double a = WARP_AS_NUM(pop(vm));
                push(vm, WARP_NUM_VAL(a + b));
            } else {
                runtime_error(vm, "invalid operands to `+'");
                return WARP_RUNTIME_ERROR;
            }
            break;
        
        
        // BINARY(NUM, +); break;
        case OP_SUB: BINARY(NUM, -); break;
        case OP_MUL: BINARY(NUM, *); break;
        case OP_DIV: BINARY(NUM, /); break;
        
        case OP_LT: BINARY(BOOL, <); break;
        case OP_GT: BINARY(BOOL, >); break;
        case OP_LTEQ: BINARY(BOOL, <=); break;
        case OP_GTEQ: BINARY(BOOL, >=); break;
        
        case OP_EQ: {
            warp_value_t b = pop(vm);
            warp_value_t a = pop(vm);
            push(vm, WARP_BOOL_VAL(value_equals(a, b)));
            break;
        }
		
		case OP_PRINT:
	        // term_set_fg(stdout, TERM_GREEN);
	        // printf("=> ");
	        // term_style_reset(stdout);
	        print_value(peek(vm, 0), stdout);
	        // printf("\n");
			break;
            
        case OP_RETURN:
            term_set_fg(stdout, TERM_GREEN);
            printf("=> ");
            term_style_reset(stdout);
            print_value(pop(vm), stdout);
            printf("\n");
            return WARP_OK;
        }
    }
    return WARP_OK;
#undef READ_8
#undef READ_CONST
}


warp_result_t warp_interpret(warp_vm_t *vm, const char *source, size_t length) {
    ASSERT(vm);
    ASSERT(source);
    
    chunk_t chunk;
    chunk_init(vm, &chunk);
    
    // TODO: compile-exec
    if(!compile(vm, &chunk, source, length)) {
        chunk_fini(vm, &chunk);
        return WARP_COMPILE_ERROR;
    }
    vm->chunk = &chunk;
    vm->ip = vm->chunk->code;
    
    warp_result_t result = warp_run(vm);
    
    chunk_fini(vm, &chunk);
    return result;
}
