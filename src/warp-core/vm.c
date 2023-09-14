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

static void reset_stack(warp_vm_t *vm) {
    vm->sp = vm->stack;
}

static void println(warp_vm_t *vm, warp_value_t *slots) {
    UNUSED(vm);
    warp_print_value(slots[0], stdout);
    putc('\n', stdout);
}

warp_vm_t *warp_vm_new(const warp_cfg_t *cfg) {
    ASSERT(cfg);
    allocator_t alloc = cfg->allocator ? cfg->allocator : default_allocator;
    warp_vm_t *vm = alloc(NULL, sizeof(warp_vm_t));
    CHECK(vm);
    
    vm->frame_count = 0;
    
    vm->allocator = alloc;
    vm->objects = NULL;
    vm->strings = warp_map_new(vm);
    vm->globals = warp_map_new(vm);
    
    reset_stack(vm);
    
    warp_register_native(vm, "println", 1, &println);
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
    call_frame_t *frame = &vm->frames[vm->frame_count-1];
    
    size_t instruction = frame->ip - frame->fn->chunk.code;
    int line = frame->fn->chunk.lines[instruction];

    fprintf(stderr, "runtime error on line %d: ", line);
    
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    reset_stack(vm);
    fputc('\n', stderr);
}

static bool invoke(warp_vm_t *vm, warp_fn_t *fn, uint8_t arg_count) {
    if(arg_count != fn->arity) {
        runtime_error(vm, "calling %s() with %d arguments, %d required",
            fn->name ? fn->name->data : "<script>",
            (int)arg_count, (int)fn->arity);
        return false;
    }
    
    if(vm->frame_count == MAX_FRAMES) {
        runtime_error(vm, "stack overflow");
        return false;
    }
    
    call_frame_t *frame = &vm->frames[vm->frame_count++];
    frame->fn = fn;
    frame->ip = fn->chunk.code;
    frame->slots = vm->sp - (arg_count + 1);
    return true;
}

static bool invoke_native(warp_vm_t *vm, warp_native_t *fn, uint8_t arg_count) {
    if(arg_count != fn->arity) {
        runtime_error(vm, "calling %s() with %d arguments, %d required",
            fn->name ? fn->name->data : "<script>",
            (int)arg_count, (int)fn->arity);
        return false;
    }
    warp_value_t *slots = vm->sp - arg_count;
    fn->native(vm, slots);
    warp_value_t result = slots[0];
    vm->sp -= arg_count + 1;
    push(vm, result);
    return true;
}

static bool invoke_val(warp_vm_t *vm, warp_value_t val, uint8_t arg_count) {
    if(WARP_IS_OBJ(val)) {
        switch(WARP_OBJ_KIND(val)) {
        case WARP_OBJ_FN:
            return invoke(vm, WARP_AS_FN(val), arg_count);
        case WARP_OBJ_NATIVE:
            return invoke_native(vm, WARP_AS_NATIVE(val), arg_count);
        default:
            break;
        }
    }
    runtime_error(vm, "cannot call non-function value");
    return false;
}

static void concatenate(warp_vm_t *vm) {
    warp_str_t *b = WARP_AS_STR(pop(vm));
    warp_str_t *a = WARP_AS_STR(pop(vm));
    
    push(vm, WARP_OBJ_VAL(warp_concat_str(vm, a, b)));
}

void dbg(warp_value_t v) {
    warp_print_value(v, stdout);
}

warp_result_t warp_run(warp_vm_t *vm) {
    ASSERT(vm);
    // reset_stack(vm);
    call_frame_t *frame = &vm->frames[vm->frame_count - 1];
    
#define READ_8() (*frame->ip++)
#define READ_16() \
    (frame->ip += 2, \
    (uint16_t)(frame->ip[-2] | (frame->ip[-1] << 8)))
#define READ_CONST() \
    (frame->fn->chunk.constants.data[READ_8()])
    
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
        warp_opcode_t instr;
#if DEBUG_TRACE_EXEC == 1
        fprintf(stdout, "\n===\n");
        for(warp_value_t *v = vm->stack; v != vm->sp; ++v) {
            if(v == frame->slots) {
                printf("...\n");
            }
            printf("[");
            warp_print_value(*v, stdout);
            printf("]\n");
        }
        fprintf(stdout, "---\n");
        disassemble_instr(&frame->fn->chunk, (int)(frame->ip - frame->fn->chunk.code), stdout);
        fprintf(stdout, "===\n");
#endif
        switch(instr = READ_8()) {            
        case OP_CONST:
            push(vm, READ_CONST());
            break;
            
        case OP_DEF_GLOB: {
            warp_value_t name = READ_CONST();
            warp_map_set(vm, vm->globals, name, peek(vm, 0));
            break;
        }
        
        case OP_GET_GLOB: {
            warp_value_t name = READ_CONST();
            warp_value_t val = WARP_NIL_VAL;
            if(!warp_map_get(vm->globals, name, &val)) {
                runtime_error(vm, "undefined global variable '%s'", WARP_AS_CSTR(name));
                return WARP_RUNTIME_ERROR;
            }
            push(vm, val);
            break;
        }
        
        case OP_SET_GLOB: {
            warp_value_t name = READ_CONST();
            if(!warp_map_set(vm, vm->globals, name, peek(vm, 0))) {
                warp_map_delete(vm->globals, name, NULL);
                runtime_error(vm, "undefined global variable '%s", WARP_AS_CSTR(name));
            }
            break;
        }
        
        case OP_GET_LOCAL: {
            uint8_t slot = READ_8();
            push(vm, frame->slots[slot]);
            break;
        }
        
        case OP_SET_LOCAL: {
            uint8_t slot = READ_8();
            frame->slots[slot] = peek(vm, 0);
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
            uint8_t slots = READ_16();
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
		
		case OP_LOOP: {
			frame->ip -= READ_16();
			break;
		}
		
        case OP_JMP: {
            frame->ip += READ_16();
            break;
        }
        
        case OP_JMP_FALSE: {
            uint16_t jmp = READ_16();
            if(value_is_falsey(peek(vm, 0))) frame->ip += jmp;
            break;
        }
		
		case OP_ENDLOOP:
			UNREACHABLE();
			break;
        
		case OP_PRINT:
	        warp_print_value(peek(vm, 0), stdout);
			break;
            
        case OP_CALL: {
            int arg_count = READ_8();
            if(!invoke_val(vm, peek(vm, arg_count), arg_count)) {
                return WARP_RUNTIME_ERROR;
            }
            frame = &vm->frames[vm->frame_count-1];
            break;
        }
            
        case OP_RETURN: {
            warp_value_t result = pop(vm);
            --vm->frame_count;
            
            vm->sp = frame->slots;
            push(vm, result);
            if(vm->frame_count == 0) {
                return WARP_OK;
            }
            frame = &vm->frames[vm->frame_count-1];
            break;
        }
        default:
            UNREACHABLE();
            break;
        }
        
#if DEBUG_TRACE_EXEC
        getchar();
#endif
    }
    return WARP_OK;
#undef READ_8
#undef READ_16
#undef READ_CONST
}

void warp_register_native(warp_vm_t *vm, const char *name, uint8_t arity, warp_native_f fn) {
    ASSERT(vm);
    ASSERT(name);
    ASSERT(fn);
    
    warp_native_t *native = warp_native_new(vm, name, arity, fn);
    warp_map_set(vm, vm->globals, WARP_OBJ_VAL(native->name), WARP_OBJ_VAL(native));
}

bool warp_get_slot(warp_vm_t *vm, int slot, warp_value_t *out) {
    ASSERT(vm);
    ASSERT(slot >= 0);
    ASSERT(out);
    if(vm->stack + slot >= vm->sp) return false;
    *out = vm->stack[slot];
    return true;
}

warp_result_t warp_interpret(warp_vm_t *vm, const char *fname, const char *source, size_t length) {
    ASSERT(vm);
    ASSERT(source);
    
    warp_fn_t *fn = compile(vm, fname, source, length);
    if(!fn) return WARP_COMPILE_ERROR;
    
    push(vm, WARP_OBJ_VAL(fn));
    invoke(vm, fn, 0);
    
    return warp_run(vm);
}
