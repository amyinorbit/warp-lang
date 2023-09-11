//===--------------------------------------------------------------------------------------------===
// parser.c
//
// Created by Amy Parent <amy@amyparent.com>
// Copyright (c) 2021 Amy Parent
// Licensed under the MIT License
// =^•.•^=
//===--------------------------------------------------------------------------------------------===
#include "parser.h"
#include "buffers.h"
#include "diag_impl.h"
#include <warp/obj.h>
#include <string.h>
#include <stdio.h>

static const char * token_names[] = {
    [TOK_LPAREN] = "l_paren",
    [TOK_RPAREN] = "r_paren",
    [TOK_LBRACE] = "l_brace",
    [TOK_RBRACE] = "r_brace",
    [TOK_LBRACKET] = "l_bracket",
    [TOK_RBRACKET] = "r_bracket",

    [TOK_PLUS] = "plus",
    [TOK_MINUS] = "minus",
    [TOK_SLASH] = "slash",
    [TOK_STAR] = "star",
    [TOK_STARSTAR] = "star_star",
    [TOK_PERCENT] = "percent",
    [TOK_CARET] = "caret",
    [TOK_TILDE] = "tilde",
    [TOK_AMP] = "amp",
    [TOK_PIPE] = "pipe",
    [TOK_BANG] = "bang",
    [TOK_QUESTION] = "question",
    [TOK_LT] = "less",
    [TOK_GT] = "greater",
    [TOK_EQUALS] = "equal",
    [TOK_LTEQ] = "less_equal",
    [TOK_GTEQ] = "greater_equal",
    [TOK_EQEQ] = "equal_equal",
    [TOK_PLUSEQ] = "plus_eqal",
    [TOK_MINUSEQ] = "minus_eqal",
    [TOK_STAREQ] = "star_eqal",
    [TOK_SLASHEQ] = "slash_eqal",
    [TOK_BANGEQ] = "bang_eqal",
    [TOK_LTLT] = "less_less",
    [TOK_GTGT] = "greater_greater",
    [TOK_GTGTEQ] = "greater_greater_equal",
    [TOK_AMPAMP] = "and_and",
    [TOK_PIPEPIPE] = "pipe_pipe",
    
    [TOK_SEMICOLON] = "semicolon",
    [TOK_NEWLINE] = "newline",
    
    [TOK_COLON] = "colon",
    [TOK_COMMA] = "comma",
    [TOK_DOT] = "dot",
    [TOK_ARROW] = "arrow",
    
    [TOK_NUMBER] = "number_literal",
    [TOK_STRING] = "string_literal",
    [TOK_IDENTIFIER] = "identifier",
    
    [TOK_SELF] = "self",
    [TOK_TRUE] = "true",
    [TOK_FALSE] = "false",
    [TOK_NIL] = "nil",
    [TOK_FUN] = "fun",
    [TOK_VAR] = "var",
    [TOK_LET] = "let",
    [TOK_RETURN] = "return",
    [TOK_FOR] = "for",
    [TOK_WHILE] = "while",
    [TOK_BREAK] = "break",
    [TOK_CONTINUE] = "continue",
    [TOK_IF] = "if",
    [TOK_THEN] = "then",
    [TOK_ELSE] = "else",
    [TOK_END] = "end",
    [TOK_INIT] = "init",
    [TOK_PRINT] = "print",
    
    [TOK_EOF] = "eof",
    [TOK_INVALID] = "invalid",
};

const char *token_name(token_kind_t kind) {
    return token_names[kind];
}

#if DEBUG_LEX == 1
static void print_token(const token_t *token) {
    printf("%-15s (%.*s)%s\n",
        token_name(token->kind),
        token->length,
        token->start,
        token->start_of_line ? " <line start>" : "");
}
#endif

static token_t make_token(parser_t *parser, token_kind_t kind) {
    token_t token;
    token.kind = kind;
    token.start = parser->start;
    token.length = (int)(parser->current - parser->start);
    token.line = parser->line;
    token.start_of_line = parser->start_of_line;
    parser->start_of_line = false;
    return token;
}

static token_t error_token(parser_t *parser) {
    token_t token;
    token.kind = TOK_INVALID;
    token.start = parser->start;
    token.length = 1;
    token.line = parser->line;
    token.start_of_line = parser->start_of_line;
    parser->start_of_line = false;
    return token;
}

static bool is_at_end(const parser_t *parser) {
    return *parser->current == '\0';
}

static inline const char *src_end(const parser_t *parser) {
    return parser->source.end;
}

static inline size_t src_left(const parser_t *parser) {
    return (size_t)(src_end(parser) - parser->current);
}

static unicode_scalar_t lex_advance(parser_t *parser) {
    unicode_scalar_t current = parser->copy;
    parser->current += unicode_utf8_size(parser->copy);

    uint8_t size = 0;
    parser->copy = unicode_utf8_read(parser->current, src_left(parser), &size);
    return size ? current : '\0';
}

static unicode_scalar_t lex_peek(const parser_t *parser) {
    return parser->copy;
}

static unicode_scalar_t lex_peek_next(const parser_t *parser) {
    if(is_at_end(parser)) return '\0';
    uint8_t current_size = unicode_utf8_size(parser->copy);
    uint8_t size = 0;
    unicode_scalar_t nc = unicode_utf8_read(parser->current + current_size,
                                            src_left(parser)-current_size,
                                            &size);
    return size ? nc : '\0';
}


static void skip_whitespace(parser_t *parser) {
    for(;;) {
        unicode_scalar_t c = lex_peek(parser);
        switch(c) {
        case ' ':
        case '\t':
        case '\r':
            lex_advance(parser);
            break;
            
        case '\n':
            lex_advance(parser);
            parser->line ++;
            parser->start_of_line = true;
            break;
            
        case '/':
            if(lex_peek_next(parser) == '/') {
                while(lex_peek(parser) != '\n' && !is_at_end(parser)) {
                    lex_advance(parser);
                }
            } else {
                return;
            }
            break;
            
        default:
            return;
        }
    }
}

static bool lex_match(parser_t *parser, unicode_scalar_t expected) {
    if(is_at_end(parser)) return false;
    if(lex_peek(parser) != expected) return false;
    lex_advance(parser);
    return true;
}

static inline bool is_digit(unicode_scalar_t c) {
    return (c >= '0' && c <= '9');
}


static token_t string(parser_t *parser) {
    str_buf_t str;
    str_buf_init(&str);
    
    bool in_esc_seq = false;
    while((lex_peek(parser) != '"' || in_esc_seq) && !is_at_end(parser)) {
        unicode_scalar_t c = lex_peek(parser);
        if(c == '\n') parser->line++;
        
        if(in_esc_seq) {
            switch(c) {
            case '\\': str_buf_write(parser->vm, &str, '\\'); break;
            case 'n': str_buf_write(parser->vm, &str, '\n'); break;
            case 'r': str_buf_write(parser->vm, &str, '\r'); break;
            case 't': str_buf_write(parser->vm, &str, '\t'); break;
            case 'e': str_buf_write(parser->vm, &str, '\33'); break;
            case '"': str_buf_write(parser->vm, &str, '"'); break;
            default:
                emit_diag_loc(
                    &parser->source,
                    WARP_DIAG_WARN,
                    parser->line, parser->current-1, 1,
                    "invalid string escape sequence"
                );
                break;
            }
            in_esc_seq = false;
        } else if(c == '\\') {
            in_esc_seq = true;
        } else {
            // TODO: There needs to be a better way of adding unicode to a string
            char data[8];
            int size = unicode_utf8_write(c, data, 8);
            for(int i = 0; i < size; ++i) {
                str_buf_write(parser->vm, &str, data[i]);
            }
        }
        lex_advance(parser);
    }
    
    if(is_at_end(parser)) {
        token_t tok = error_token(parser);
        emit_diag(&parser->source, WARP_DIAG_ERROR, &tok, "unterminated character string");
        str_buf_fini(parser->vm, &str);
        return tok;
    }
    
    lex_advance(parser); // We make sure to eat the closing quote
    token_t tok = make_token(parser, TOK_STRING);
    tok.value = WARP_OBJ_VAL(warp_copy_c_str(parser->vm, str.data, str.count));
    str_buf_fini(parser->vm, &str);
    return tok;
}

static token_t number(parser_t *parser) {
    while(is_digit(lex_peek(parser))) {
        lex_advance(parser);
    }
    
    if(lex_peek(parser) == '.' && is_digit(lex_peek_next(parser))) {
        lex_advance(parser);
        while(is_digit(lex_peek(parser))) {
            lex_advance(parser);
        }
    }
    
    token_t tok = make_token(parser, TOK_NUMBER);
    tok.value = WARP_NUM_VAL(strtod(parser->start, NULL));
    return tok;
}

static token_kind_t check_keyword(
    const parser_t *parser,
    int start,
    int length,
    const char *rest,
    token_kind_t kind
) {
    if(parser->current - parser->start != start + length)
        return TOK_IDENTIFIER;
    if(memcmp(parser->start + start, rest, length))
        return TOK_IDENTIFIER;
    return kind;
}

static token_kind_t identifier_kind(const parser_t *parser) {
    switch(parser->start[0]) {
    case 'b': return check_keyword(parser, 1, 4, "reak", TOK_BREAK);
    case 'c': return check_keyword(parser, 1, 7, "ontinue", TOK_CONTINUE);
    case 'e': return check_keyword(parser, 1, 3, "lse", TOK_ELSE);
    case 'f':
        if (parser->current - parser->start > 1) {
            switch(parser->start[1]) {
                case 'a': return check_keyword(parser, 2, 3, "lse", TOK_FALSE);
                case 'o': return check_keyword(parser, 2, 1, "r", TOK_FOR);
                case 'u': return check_keyword(parser, 2, 1, "n", TOK_FUN);
            }
        }
        break;    
    case 'i':
        if (parser->current - parser->start > 1) {
            switch(parser->start[1]) {
                case 'f': return check_keyword(parser, 2, 0, "", TOK_IF);
                case 'n': return check_keyword(parser, 2, 2, "it", TOK_INIT);
            }
        }
        break;
    case 'l': return check_keyword(parser, 1, 2, "et", TOK_LET);
    case 'n': return check_keyword(parser, 1, 2, "il", TOK_NIL);
    case 'p': return check_keyword(parser, 1, 4, "rint", TOK_PRINT);
    case 'r': return check_keyword(parser, 1, 5, "eturn", TOK_RETURN);
    case 't': return check_keyword(parser, 1, 3, "rue", TOK_TRUE);
    case 'v': return check_keyword(parser, 1, 2, "ar", TOK_VAR);
    case 'w': return check_keyword(parser, 1, 4, "hile", TOK_WHILE);
    }
    return TOK_IDENTIFIER;
}

static token_t identifier(parser_t *parser) {
    while(unicode_is_identifier(lex_peek(parser))) {
        lex_advance(parser);
    }
    return make_token(parser, identifier_kind(parser));
}

token_t scan_token(parser_t *parser) {
    skip_whitespace(parser);
    parser->start = parser->current;
    if(is_at_end(parser)) return make_token(parser, TOK_EOF);
    
    unicode_scalar_t c = lex_advance(parser);
    

    if(is_digit(c)) return number(parser);
    if(unicode_is_identifier_head(c)) return identifier(parser);
    
    switch(c) {
        case ';': return make_token(parser, TOK_SEMICOLON);
        
        case '{': return make_token(parser, TOK_LBRACE);
        case '}': return make_token(parser, TOK_RBRACE);
        case '[': return make_token(parser, TOK_LBRACKET);
        case ']': return make_token(parser, TOK_RBRACKET);
        case '(': return make_token(parser, TOK_LPAREN);
        case ')': return make_token(parser, TOK_RPAREN);
        case ':': return make_token(parser, TOK_COLON);
        case '.': return make_token(parser, TOK_DOT);
        case ',': return make_token(parser, TOK_COMMA);

        case '^': return make_token(parser, TOK_CARET);
        case '~': return make_token(parser, TOK_TILDE);
        case '%': return make_token(parser, TOK_PERCENT);
        case '?': return make_token(parser, TOK_QUESTION);
        case '"': return string(parser);
        
        case '=': return make_token(parser, lex_match(parser, '=') ? TOK_EQEQ : TOK_EQUALS);
        case '!': return make_token(parser, lex_match(parser, '=') ? TOK_BANGEQ : TOK_BANG);
        case '>': return make_token(parser, lex_match(parser, '=') ? TOK_GTEQ : TOK_GT);
        case '<': return make_token(parser, lex_match(parser, '=') ? TOK_LTEQ : TOK_LT);
        
        case '+': return make_token(parser, lex_match(parser, '=') ? TOK_PLUSEQ : TOK_PLUS);
        case '*': return make_token(parser, lex_match(parser, '=') ? TOK_STAREQ : TOK_STAR);
        case '/': return make_token(parser, lex_match(parser, '=') ? TOK_SLASHEQ : TOK_SLASH);
        case '-':
            if(lex_match(parser, '=')) return make_token(parser, TOK_MINUSEQ);
            else if(lex_match(parser, '>')) return make_token(parser, TOK_ARROW);
            return make_token(parser, TOK_MINUS);
    }
    
    token_t tok = error_token(parser);
    emit_diag(&parser->source, WARP_DIAG_ERROR, &tok, "invalid character");
    return tok;
}

void parser_init_text(parser_t *parser, warp_vm_t *vm, const char *text, size_t length) {
    UNUSED(parser);
    UNUSED(text);
    parser->vm = vm;
    
    parser->source.fname = "<repl>";
    parser->source.start = text;
    parser->source.end = text + length;
    parser->start = text;
    parser->current = text;
    parser->line = 1;
    parser->start_of_line = true;

    parser->panic = false;
    parser->had_error = false;
    
    uint8_t size = 0;
    parser->copy = unicode_utf8_read(text, src_left(parser), &size);
    if(!size) parser->copy = '\0';
}

// MARK: - Parser implementation

void error_silent(parser_t *parser) {
    parser->panic = true;
    parser->had_error = true;
}

void error_at_varg(parser_t *parser, const token_t *token, const char *fmt, va_list args) {
    if(parser->panic) return;
    emit_diag_varg(&parser->source, WARP_DIAG_ERROR, token, fmt, args);
    parser->panic = true;
    parser->had_error = true;
}

token_t *previous(parser_t *parser) { return &parser->previous_token; }
token_t *current(parser_t *parser) { return &parser->current_token; }

void error_at(parser_t *parser, const token_t *token, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    error_at_varg(parser, token, fmt, args);
    va_end(args);
}

void synchronize(parser_t *parser) {
    if(!parser->panic) return;
    parser->panic = false;
    
    while(current(parser)->kind != TOK_EOF) {
        if(previous(parser)->kind == TOK_SEMICOLON) return;
        
        switch(current(parser)->kind) {
        case TOK_FUN:
        case TOK_VAR:
        case TOK_FOR:
        case TOK_IF:
        case TOK_WHILE:
        case TOK_RETURN:
            return;
            
        default:
            break;
        }
        
        advance(parser);
    }
}

bool check_terminator(parser_t *parser) {
    return current(parser)->start_of_line
        || check(parser, TOK_EOF)
        || check(parser, TOK_RBRACE);
}

bool check(parser_t *parser, token_kind_t kind) {
    return current(parser)->kind == kind;
}

bool match(parser_t *parser, token_kind_t kind) {
    if(!check(parser, kind)) return false;
    advance(parser);
    return true;
}

void advance(parser_t *parser) {
    *previous(parser) = *current(parser);
    for(;;) {
        *current(parser) = scan_token(parser);
#if DEBUG_LEX
        print_token(current(parser));
#endif
        if(current(parser)->kind != TOK_INVALID) break;
    }
}

void consume_terminator(parser_t *parser, const char *msg) {
    if(match(parser, TOK_SEMICOLON) || check_terminator(parser)) return;
    error_at(parser, current(parser), msg);
}

void consume(parser_t *parser, token_kind_t kind, const char *msg) {
    if(current(parser)->kind == kind) {
        advance(parser);
        return;
    }
    if(current(parser)->kind != TOK_INVALID) {
        error_at(parser, current(parser), msg);
    } else {
        error_silent(parser);
    }
}
