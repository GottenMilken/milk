#ifndef MILK_LEXER_H
#define MILK_LEXER_H

#include "token.h"

#define MILK_MAX_INDENTS 256

typedef struct {
    const char *source;
    const char *current;

    int line;
    int column;

    int at_line_start;
    int pending_indent;
    int indent_stack[MILK_MAX_INDENTS];
    int indent_count;

    int pending_dedents;
    int finished;
} Lexer;

void lexer_init(Lexer *lexer, const char *source);
Token lexer_next(Lexer *lexer);

#endif
