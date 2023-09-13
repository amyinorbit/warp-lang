//===--------------------------------------------------------------------------------------------===
// compiler.c
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#include <warp/instr.h>
#include <warp/obj.h>
#include "compiler.h"
#include "parser.h"
#include "types/obj_impl.h"
#include "diag_impl.h"
#include "debug.h"
#include <string.h>

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY
} precedence_t;

#define UINT8_COUNT (UINT8_MAX + 1)

typedef struct local_t local_t;
typedef struct compiler_t compiler_t;
typedef struct loop_t loop_t;

struct local_t {
    token_t     name;
    int         depth;
};

struct loop_t {
    loop_t      *enclosing;
    int         start;
    int         body;
    int         exit_jmp;
    int         scope_depth;
};

struct compiler_t {
    // compiler_t  *enclosing;
    warp_vm_t   *vm;
    chunk_t     *chunk;
    parser_t    *parser;
    
    loop_t      *loop;
    
    local_t     locals[UINT8_COUNT];
    int         local_count;
    int         scope_depth;
    
    int         num_slots;
    int         max_slots;
};

typedef void (*parse_fn_t)(compiler_t *comp, bool can_assign);

typedef struct {
    parse_fn_t      prefix;
    parse_fn_t      infix;
    precedence_t    precedence;
} parse_rule_t;


#define WARP_OP(code, _, effect) [OP_##code] = effect,
static int stack_effect[] = {
#include <warp/instr.def>
};
#undef WARP_OP

#define WARP_OP(code, op_size, _) [OP_##code] = op_size+1,
static int code_size[] = {
#include <warp/instr.def>
};
#undef WARP_OP

static chunk_t *current_chunk(compiler_t *comp) {
    return comp->chunk;
}

static void emit_byte(compiler_t *comp, uint8_t byte) {
    chunk_write(comp->vm, current_chunk(comp), byte, previous(comp->parser)->line);
}

static void emit_instr(compiler_t *comp, uint8_t instr) {
    if(instr != OP_BLOCK) {
        comp->num_slots += stack_effect[instr];
        if(comp->num_slots > comp->max_slots) {
            comp->max_slots = comp->num_slots;
        }
    }
    emit_byte(comp, instr);
}

static void emit_bytes(compiler_t *comp, uint8_t byte1, uint8_t byte2) {
    emit_instr(comp, byte1);
    emit_byte(comp, byte2);
}

static void emit_bytes_long(compiler_t *comp, uint8_t instr, uint16_t operand) {
    emit_instr(comp, instr);
    emit_byte(comp, operand & 0x00ff);
    emit_byte(comp, operand >> 8);
}

static int emit_jump(compiler_t *comp, uint8_t instr) {
    emit_bytes_long(comp, instr, 0xffff);
    return current_chunk(comp)->count - 2;
}

static void emit_loop(compiler_t *comp, int start) {
    emit_instr(comp, OP_LOOP);
    int jmp = current_chunk(comp)->count - start + 2;
    if(jmp > UINT16_MAX) {
        error_at(comp->parser, previous(comp->parser), "too much code to jump over");
    }
    emit_byte(comp, jmp & 0xff);
    emit_byte(comp, (jmp >> 8) & 0xff);
}

static void patch_jump(compiler_t *comp, int offset) {
    int jmp = current_chunk(comp)->count - offset - 2;
    
    if(jmp > UINT16_MAX) {
        error_at(comp->parser, previous(comp->parser), "too much code to jump over");
    }
    current_chunk(comp)->code[offset] = jmp & 0xff;
    current_chunk(comp)->code[offset+1] = (jmp >> 8) & 0xff;
}

static void emit_return(compiler_t *comp) {
    emit_instr(comp, OP_RETURN);
}

static void emit_const(compiler_t *comp, warp_value_t value) {
    int idx = chunk_add_const(comp->vm, current_chunk(comp), value);
    if(idx < UINT8_MAX) {
        emit_bytes(comp, OP_CONST, (uint8_t)idx);
    } else {
        error_at(comp->parser, previous(comp->parser), "too many constants in one bytecode unit");
    }
}

static inline int add_ident_const(compiler_t *comp, const token_t *name) {
    int idx = chunk_add_const(
        comp->vm,
        current_chunk(comp),
        WARP_OBJ_VAL(warp_copy_c_str(comp->vm, name->start, name->length))
    );
    if(idx >= UINT8_MAX) {
        error_at(comp->parser, previous(comp->parser), "too many constants in one bytecode unit");
    }
    return idx;
}

static void begin_scope(compiler_t *comp) {
    comp->scope_depth += 1;
}

static int drop_locals(compiler_t *comp, int target_scope_depth) {
    int num_slots = 0;
    while(comp->local_count > 0 && comp->locals[comp->local_count-1].depth > target_scope_depth) {
        comp->local_count -= 1;
        num_slots += 1;
    }
    ASSERT(num_slots < UINT16_MAX);
    emit_bytes_long(comp, OP_BLOCK, num_slots);
    return num_slots;
}

static void end_scope(compiler_t *comp) {
    comp->scope_depth -= 1;
    comp->num_slots -= drop_locals(comp, comp->scope_depth);
}

static void open_loop(compiler_t *comp, loop_t *loop) {
    ASSERT(loop != NULL);
    begin_scope(comp);
    loop->enclosing = comp->loop;
    loop->start = current_chunk(comp)->count - 1;
    loop->body = -1;
    loop->exit_jmp = -1;
    loop->scope_depth = comp->scope_depth;
    comp->loop = loop;
}

static void test_loop_jump(compiler_t *comp) {
    ASSERT(comp->loop != NULL);
    ASSERT(comp->loop->exit_jmp == -1);
    comp->loop->exit_jmp = emit_jump(comp, OP_JMP_FALSE);
}

static void start_loop_body(compiler_t *comp) {
    ASSERT(comp->loop != NULL);
    ASSERT(comp->loop->body == -1);
    comp->loop->body = current_chunk(comp)->count;
}

static void close_loop(compiler_t *comp) {
    ASSERT(comp->loop != NULL);
    loop_t *loop = comp->loop;
    ASSERT(loop->start >= 0);
    ASSERT(loop->body >= 0);
    ASSERT(loop->exit_jmp >= 0);
    
    int i = loop->body;
    while(i < current_chunk(comp)->count) {
        uint8_t instr = current_chunk(comp)->code[i];
        if(instr != OP_ENDLOOP) {
            i += code_size[instr];
            continue;
        }
        current_chunk(comp)->code[i] = OP_JMP;
    }
    comp->loop = loop->enclosing;
    end_scope(comp);
    
    emit_loop(comp, loop->start);
    patch_jump(comp, loop->exit_jmp);
    comp->loop = loop->enclosing;
}

static void end_compiler(compiler_t *comp) {
    emit_return(comp);
}

static void expression(compiler_t *comp);
static void declaration(compiler_t *comp);
static const parse_rule_t *get_rule(token_kind_t kind);
static void parse_precedence(compiler_t *comp, precedence_t prec);

static void number(compiler_t *comp, bool can_assign) {
    UNUSED(can_assign);
    emit_const(comp, previous(comp->parser)->value);
}

static void string(compiler_t *comp, bool can_assign) {
    UNUSED(can_assign);
    emit_const(comp, previous(comp->parser)->value);
}

static void literal(compiler_t *comp, bool can_assign) {
    UNUSED(can_assign);
    token_t tok = *previous(comp->parser);
    switch(tok.kind) {
        case TOK_TRUE: emit_instr(comp, OP_TRUE); break;
        case TOK_FALSE: emit_instr(comp, OP_FALSE); break;
        case TOK_NIL: emit_instr(comp, OP_NIL); break;
        default: UNREACHABLE(); break;
    }
}

static bool ident_equals(const token_t *a, const token_t *b) {
    if(a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

static int resolve_local(compiler_t *comp, const token_t *name) {
    for(int i = comp->local_count - 1; i >= 0; --i) {
        local_t *local = &comp->locals[i];
        if(ident_equals(name, &local->name)) {
            if(local->depth == -1) {
                error_at(
                    comp->parser,
                    name,
                    "variable '%.*s' used in its own initializer",
                    name->length,
                    name->start
                );
            }
            return i;
        } 
    }
    return -1;
}

static void named_variable(compiler_t *comp, const token_t *name, bool can_assign) {
    
    uint8_t get_op, set_op;
    int arg = resolve_local(comp, name);
    if(arg != -1) {
        get_op = OP_GET_LOCAL;
        set_op = OP_SET_LOCAL;
    } else {
        arg = add_ident_const(comp, name);
        get_op = OP_GET_GLOB;
        set_op = OP_SET_GLOB;
    }
    
    if(can_assign && match(comp->parser, TOK_EQUALS)) {
        expression(comp);
        emit_bytes(comp, set_op, (uint8_t)arg);
    } else {
        emit_bytes(comp, get_op, (uint8_t)arg);
    }
}

static void variable(compiler_t *comp, bool can_assign) {
    named_variable(comp, previous(comp->parser), can_assign);
}

static void expression(compiler_t *comp) {
    parse_precedence(comp, PREC_ASSIGNMENT);
}

static void unary(compiler_t *comp, bool can_assign) {
    UNUSED(can_assign);
    token_t op = *previous(comp->parser);
    
    expression(comp);    
    switch(op.kind) {
    case TOK_MINUS: emit_instr(comp, OP_NEG); break;
    case TOK_BANG: emit_instr(comp, OP_NOT); break;
    default: UNREACHABLE(); break;
    }
}

static void binary(compiler_t *comp, bool can_assign) {
    UNUSED(can_assign);
    
    token_t op = *previous(comp->parser);
    const parse_rule_t *rule = get_rule(op.kind);
    parse_precedence(comp, rule->precedence + 1);
    
    switch(op.kind) {
    case TOK_PLUS: emit_instr(comp, OP_ADD); break;
    case TOK_MINUS: emit_instr(comp, OP_SUB); break;
    case TOK_STAR: emit_instr(comp, OP_MUL); break;
    case TOK_SLASH: emit_instr(comp, OP_DIV); break;
    
    case TOK_LT: emit_instr(comp, OP_LT); break;
    case TOK_GT: emit_instr(comp, OP_GT); break;
    case TOK_LTEQ: emit_instr(comp, OP_LTEQ); break;
    case TOK_GTEQ: emit_instr(comp, OP_GTEQ); break;
    case TOK_EQEQ: emit_instr(comp, OP_EQ); break;
    case TOK_BANGEQ: emit_bytes(comp, OP_EQ, OP_NOT); break;
    
    default: UNREACHABLE(); return;
    }
}

static void and_expr(compiler_t *comp, bool can_assign) {
    UNUSED(can_assign);
    
    int end_jmp = emit_jump(comp, OP_JMP_FALSE);
    emit_instr(comp, OP_POP);
    parse_precedence(comp, PREC_AND);
    patch_jump(comp, end_jmp);
}

static void or_expr(compiler_t *comp, bool can_assign) {
    UNUSED(can_assign);
    
    int else_jmp = emit_jump(comp, OP_JMP_FALSE);
    int end_jmp = emit_jump(comp, OP_JMP);
    patch_jump(comp, else_jmp);
    emit_instr(comp, OP_POP);
    parse_precedence(comp, PREC_OR);
    patch_jump(comp, end_jmp);
}

static void grouping(compiler_t *comp, bool can_assign) {
    UNUSED(can_assign);
    expression(comp);
    consume(comp->parser, TOK_RPAREN, "missing ')' after expression");
}

static bool check_end_block(parser_t *parser) {
    return check(parser, TOK_RBRACE) || check(parser, TOK_EOF);
}

static void block_body(compiler_t *comp) {
    // If we have an empty block, we must still make sure we return nil from that block;
    if(check_end_block(comp->parser)) {
        emit_instr(comp, OP_NIL);
    }
    while(!check_end_block(comp->parser)) {
        declaration(comp);
    }
    consume(comp->parser, TOK_RBRACE, "missing '}' after block");
}

static void block(compiler_t *comp, bool can_assign) {
    UNUSED(can_assign);
    begin_scope(comp);
    block_body(comp);
    end_scope(comp);
}

static void if_expr(compiler_t *comp, bool can_assign);

static void if_then_expr(compiler_t *comp, int then_jmp) {
    emit_instr(comp, OP_POP);
    expression(comp);
    int else_jmp = emit_jump(comp, OP_JMP);
    patch_jump(comp, then_jmp);
    
    if(match(comp->parser, TOK_ELSE)) {
        emit_instr(comp, OP_POP);
        expression(comp);
    } else {
        emit_instr(comp, OP_NIL);
    }
    consume(comp->parser, TOK_END, "missing 'end' after if-else expression");
    patch_jump(comp, else_jmp);
}

static void if_brace_expr(compiler_t *comp, int then_jmp) {
    emit_instr(comp, OP_POP);
    block(comp, false);
    int else_jmp = emit_jump(comp, OP_JMP);
    patch_jump(comp, then_jmp);
    
    if(match(comp->parser, TOK_ELSE)) {
        emit_instr(comp, OP_POP);
        if(match(comp->parser, TOK_LBRACE)) {
            block(comp, false);
        } else if(match(comp->parser, TOK_IF)) {
            if_expr(comp, false);
        } else {
            error_at(comp->parser, current(comp->parser), "missing else expression");
        }
    } else {
        emit_instr(comp, OP_NIL);
    }
    patch_jump(comp, else_jmp);
}

static void if_expr(compiler_t *comp, bool can_assign) {
    UNUSED(can_assign);
    expression(comp);
    int then_jmp = emit_jump(comp, OP_JMP_FALSE);
    
    if(match(comp->parser, TOK_LBRACE)) {
        if_brace_expr(comp, then_jmp);
    } else if(match(comp->parser, TOK_THEN)) {
        if_then_expr(comp, then_jmp);
    } else {
        error_at(comp->parser, current(comp->parser), "missing expression after 'if'");
    }
}

static void while_expr(compiler_t *comp, bool can_assign) {
    UNUSED(can_assign);
    
    loop_t loop;
    open_loop(comp, &loop);
    expression(comp);
    test_loop_jump(comp);
    start_loop_body(comp);
    
    emit_instr(comp, OP_POP);
    consume(comp->parser, TOK_LBRACE, "missing loop body");
    block_body(comp);
    
    close_loop(comp);
}

static void print(compiler_t *comp, bool can_assign) {
    UNUSED(can_assign);
    expression(comp);
    emit_instr(comp, OP_PRINT);
}

const parse_rule_t rules[] = {
    [TOK_LPAREN] =      {grouping,  NULL,       PREC_NONE},
    [TOK_RPAREN] =      {NULL,      NULL,       PREC_NONE},
    [TOK_LBRACE] =      {block,     NULL,       PREC_NONE},
    [TOK_RBRACE] =      {NULL,      NULL,       PREC_NONE},
    [TOK_LBRACKET] =    {NULL,      NULL,       PREC_NONE},
    [TOK_RBRACKET] =    {NULL,      NULL,       PREC_NONE},
    [TOK_PLUS] =        {NULL,      binary,     PREC_TERM},
    [TOK_MINUS] =       {unary,     binary,     PREC_TERM},
    [TOK_SLASH] =       {NULL,      binary,     PREC_FACTOR},
    [TOK_STAR] =        {NULL,      binary,     PREC_FACTOR},
    [TOK_STARSTAR] =    {NULL,      NULL,       PREC_NONE},
    [TOK_PERCENT] =     {NULL,      NULL,       PREC_NONE},
    [TOK_CARET] =       {NULL,      NULL,       PREC_NONE},
    [TOK_TILDE] =       {NULL,      NULL,       PREC_NONE},
    [TOK_AMP] =         {NULL,      NULL,       PREC_NONE},
    [TOK_PIPE] =        {NULL,      NULL,       PREC_NONE},
    [TOK_BANG] =        {unary,     NULL,       PREC_UNARY},
    [TOK_QUESTION] =    {NULL,      NULL,       PREC_NONE},
    [TOK_LT] =          {NULL,      binary,     PREC_COMPARISON},
    [TOK_GT] =          {NULL,      binary,     PREC_COMPARISON},
    [TOK_EQUALS] =      {NULL,      NULL,       PREC_NONE},
    [TOK_LTEQ] =        {NULL,      binary,     PREC_COMPARISON},
    [TOK_GTEQ] =        {NULL,      binary,     PREC_COMPARISON},
    [TOK_EQEQ] =        {NULL,      binary,     PREC_EQUALITY},
    [TOK_PLUSEQ] =      {NULL,      NULL,       PREC_NONE},
    [TOK_MINUSEQ] =     {NULL,      NULL,       PREC_NONE},
    [TOK_STAREQ] =      {NULL,      NULL,       PREC_NONE},
    [TOK_SLASHEQ] =     {NULL,      NULL,       PREC_NONE},
    [TOK_BANGEQ] =      {NULL,      binary,     PREC_EQUALITY},
    [TOK_LTLT] =        {NULL,      NULL,       PREC_NONE},
    [TOK_GTGT] =        {NULL,      NULL,       PREC_NONE},
    [TOK_GTGTEQ] =      {NULL,      NULL,       PREC_NONE},
    [TOK_AMPAMP] =      {NULL,      and_expr,   PREC_AND},
    [TOK_PIPEPIPE] =    {NULL,      or_expr,    PREC_OR},
    [TOK_SEMICOLON] =   {NULL,      NULL,       PREC_NONE},
    [TOK_NEWLINE] =     {NULL,      NULL,       PREC_NONE},
    [TOK_COLON] =       {NULL,      NULL,       PREC_NONE},
    [TOK_COMMA] =       {NULL,      NULL,       PREC_NONE},
    [TOK_DOT] =         {NULL,      NULL,       PREC_NONE},
    [TOK_ARROW] =       {NULL,      NULL,       PREC_NONE},
    [TOK_NUMBER] =      {number,    NULL,       PREC_NONE},
    [TOK_STRING] =      {string,    NULL,       PREC_NONE},
    [TOK_IDENTIFIER] =  {variable,  NULL,       PREC_NONE},
    [TOK_SELF] =        {NULL,      NULL,       PREC_NONE},
    [TOK_TRUE] =        {literal,   NULL,       PREC_NONE},
    [TOK_FALSE] =       {literal,   NULL,       PREC_NONE},
    [TOK_NIL] =         {literal,   NULL,       PREC_NONE},
    [TOK_FUN] =         {NULL,      NULL,       PREC_NONE},
    [TOK_VAR] =         {NULL,      NULL,       PREC_NONE},
    [TOK_LET] =         {NULL,      NULL,       PREC_NONE},
    [TOK_RETURN] =      {NULL,      NULL,       PREC_NONE},
    [TOK_FOR] =         {NULL,      NULL,       PREC_NONE},
    [TOK_WHILE] =       {while_expr,NULL,       PREC_NONE},
    [TOK_BREAK] =       {NULL,      NULL,       PREC_NONE},
    [TOK_CONTINUE] =    {NULL,      NULL,       PREC_NONE},
    [TOK_IF] =          {if_expr,   NULL,       PREC_NONE},
    [TOK_THEN] =        {NULL,      NULL,       PREC_NONE},
    [TOK_ELSE] =        {NULL,      NULL,       PREC_NONE},
    [TOK_END] =         {NULL,      NULL,       PREC_NONE},
    [TOK_INIT] =        {NULL,      NULL,       PREC_NONE},
    [TOK_PRINT] =       {print,     NULL,       PREC_NONE},
    [TOK_EOF] =         {NULL,      NULL,       PREC_NONE},
    [TOK_INVALID] =     {NULL,      NULL,       PREC_NONE},
};

static void parse_precedence(compiler_t *comp, precedence_t prec) {
    advance(comp->parser);
    parse_fn_t prefix = get_rule(previous(comp->parser)->kind)->prefix;
    if(!prefix) {
        error_at(comp->parser, previous(comp->parser), "expected an expression");
        return;
    }
    
    bool can_assign = prec <= PREC_ASSIGNMENT;
    prefix(comp, can_assign);
    
    if(!can_assign && check(comp->parser, TOK_EQUALS)) {
        error_at(comp->parser, current(comp->parser), "cannot assign to non-variable expression");
    }
    
    while(prec <= get_rule(current(comp->parser)->kind)->precedence) {
        advance(comp->parser);
        parse_fn_t infix = get_rule(previous(comp->parser)->kind)->infix;
        infix(comp, can_assign);
    }
}

static const parse_rule_t *get_rule(token_kind_t kind) {
    return &rules[kind];
}

static void add_local(compiler_t *comp, const token_t *name) {
    if(comp->local_count > UINT8_MAX) {
        error_at(comp->parser, name, "too many local variables in function");
        return;
    }
    local_t *local = &comp->locals[comp->local_count++];
    local->name = *name;
    local->depth = -1; //comp->scope_depth;
}


static void declare_variable(compiler_t *comp) {
    if(comp->scope_depth == 0) return;
    
    const token_t *name = previous(comp->parser);
    
    for(int i = comp->local_count - 1; i >= 0; --i) {
        local_t *local = &comp->locals[i];
        if(local->depth != -1 && local->depth != comp->scope_depth)
            break;
        if(ident_equals(name, &local->name)) {
            error_at(
                comp->parser,
                name,
                "local variable '%.*s' already defined",
                name->length,
                name->start
            );
        }
    }
    add_local(comp, name);
}

static void define_variable(compiler_t *comp, int idx) {
    if(comp->scope_depth > 0) {
        comp->locals[comp->local_count - 1].depth = comp->scope_depth;
        /*
        Because we're expression-oriented, every expression must leave its results
        on top of the stack. This includes variable declarations, which, because of the
        way the compiler is written, don't. So we need a GET_LOCAL instruction here so
        we leave the stack as the rest of the language expects it.
        */
        emit_instr(comp, OP_DUP);
        return;
    }
    emit_bytes(comp, OP_DEF_GLOB, idx);
}

static int parse_variable(compiler_t *comp, const char *msg) {
    consume(comp->parser, TOK_IDENTIFIER, msg);
    
    declare_variable(comp);
    if(comp->scope_depth > 0) return 0;
    
    return add_ident_const(comp, previous(comp->parser));
}

static void var_decl_stmt(compiler_t *comp) {
    int global = parse_variable(comp, "expected a variable name");
    
    consume(comp->parser, TOK_EQUALS, "missing variable initializer");
    expression(comp);
    
    define_variable(comp, global);
}

static void declaration(compiler_t *comp) {
    if(match(comp->parser, TOK_VAR)) {
        var_decl_stmt(comp);
        consume_terminator(comp->parser, "expected a line return or a semicolon");
    } else {
        expression(comp);
        consume_terminator(comp->parser, "expected a line return or a semicolon");
    }
    
    if(!check_end_block(comp->parser)) {
        emit_instr(comp, OP_POP);
    }
    if(comp->parser->panic) synchronize(comp->parser);
}

static void compiler_init_root(compiler_t *compiler, warp_vm_t *vm, parser_t *parser, chunk_t *chunk) {
    compiler->vm = vm;
    compiler->parser = parser;
    
    compiler->chunk = chunk;
    compiler->local_count = 0;
    compiler->scope_depth = 0;
    compiler->num_slots = 0;
    compiler->max_slots = 0;
}

bool compile(warp_vm_t *vm, chunk_t *chunk, const char *src, size_t length) {
    compiler_t comp;
    
    parser_t parser;
    parser_init_text(&parser, vm, src, length);
    compiler_init_root(&comp, vm, &parser, chunk);
    
    advance(comp.parser);
    while(!match(comp.parser, TOK_EOF)) {
        declaration(&comp);
    }
    end_compiler(&comp);
    
    consume(comp.parser, TOK_EOF, "expected end of expression");
    
#if DEBUG_PRINT_CODE == 1
    if(!parser.had_error) {
        disassemble_chunk(chunk, "compiled code", stdout);
    }
#endif
    
    return !parser.had_error;
}

