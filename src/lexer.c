#include "lexer.h"

#include <ctype.h>
#include <string.h>

static Token make_token(
Lexer *lexer,
TokenType type,
const char *start,
int line,
int column,
int indent
)
{
Token token;

token.type = type;
token.start = start;
token.length = (int)(lexer->current - start);
token.line = line;
token.column = column;
token.indent = indent;

return token;

}

static Token make_empty_token(
Lexer *lexer,
TokenType type
)
{
Token token;

token.type = type;
token.start = lexer->current;
token.length = 0;
token.line = lexer->line;
token.column = lexer->column;
token.indent =
    lexer->indent_stack[
        lexer->indent_count - 1
    ];

return token;

}

static void advance_char(Lexer *lexer)
{
if (*lexer->current == '\n') {
lexer->line++;
lexer->column = 1;
lexer->at_line_start = 1;
} else {
lexer->column++;
}

lexer->current++;

}

static TokenType keyword_type(
const char *start,
int length
)
{
if (length == 4 &&
strncmp(start, "show", 4) == 0)
return TOKEN_SHOW;

if (length == 6 &&
    strncmp(start, "during", 6) == 0)
    return TOKEN_DURING;

if (length == 5 &&
    strncmp(start, "check", 5) == 0)
    return TOKEN_CHECK;

if (length == 9 &&
    strncmp(start, "otherwise", 9) == 0)
    return TOKEN_OTHERWISE;

if (length == 6 &&
    strncmp(start, "repeat", 6) == 0)
    return TOKEN_REPEAT;

if (length == 3 &&
    strncmp(start, "yes", 3) == 0)
    return TOKEN_YES;

if (length == 2 &&
    strncmp(start, "no", 2) == 0)
    return TOKEN_NO;

if (length == 5 &&
    strncmp(start, "empty", 5) == 0)
    return TOKEN_EMPTY;

return TOKEN_IDENTIFIER;

}

static Token handle_indentation(Lexer *lexer)
{
int indent = 0;

while (
    *lexer->current == ' ' ||
    *lexer->current == '\t'
) {
    if (*lexer->current == '\t')
        indent += 4;
    else
        indent++;

    advance_char(lexer);
}

if (*lexer->current == '\n') {
    const char *start = lexer->current;
    int line = lexer->line;
    int column = lexer->column;

    advance_char(lexer);

    return make_token(
        lexer,
        TOKEN_NEWLINE,
        start,
        line,
        column,
        indent
    );
}

if (*lexer->current == '\0') {
    lexer->at_line_start = 0;

    while (lexer->indent_count > 1) {
        lexer->indent_count--;
        lexer->pending_dedents++;
    }

    if (lexer->pending_dedents > 0) {
        lexer->pending_dedents--;

        return make_empty_token(
            lexer,
            TOKEN_DEDENT
        );
    }

    return make_empty_token(
        lexer,
        TOKEN_EOF
    );
}

int current_indent =
    lexer->indent_stack[
        lexer->indent_count - 1
    ];

lexer->at_line_start = 0;

if (indent > current_indent) {
    if (
        lexer->indent_count >=
        MILK_MAX_INDENTS
    ) {
        return make_empty_token(
            lexer,
            TOKEN_EOF
        );
    }

    lexer->indent_stack[
        lexer->indent_count++
    ] = indent;

    return make_empty_token(
        lexer,
        TOKEN_INDENT
    );
}

if (indent < current_indent) {
    int found = 0;

    for (
        int i = lexer->indent_count - 1;
        i >= 0;
        i--
    ) {
        if (
            lexer->indent_stack[i] ==
            indent
        ) {
            found = 1;
            break;
        }
    }

    if (!found) {
        return make_empty_token(
            lexer,
            TOKEN_EOF
        );
    }

    while (
        lexer->indent_count > 1 &&
        lexer->indent_stack[
            lexer->indent_count - 1
        ] > indent
    ) {
        lexer->indent_count--;
        lexer->pending_dedents++;
    }

    if (lexer->pending_dedents > 0) {
        lexer->pending_dedents--;

        return make_empty_token(
            lexer,
            TOKEN_DEDENT
        );
    }
}

const char *start = lexer->current;
int line = lexer->line;
int column = lexer->column;

if (
    isalpha(
        (unsigned char)*lexer->current
    ) ||
    *lexer->current == '_'
) {
    while (
        isalnum(
            (unsigned char)*lexer->current
        ) ||
        *lexer->current == '_'
    ) {
        advance_char(lexer);
    }

    int length =
        (int)(lexer->current - start);

    return make_token(
        lexer,
        keyword_type(start, length),
        start,
        line,
        column,
        indent
    );
}

return make_token(
    lexer,
    TOKEN_EOF,
    start,
    line,
    column,
    indent
);

}

void lexer_init(
Lexer *lexer,
const char *source
)
{
memset(
lexer,
0,
sizeof(Lexer)
);

lexer->source = source;
lexer->current = source;
lexer->line = 1;
lexer->column = 1;
lexer->at_line_start = 1;
lexer->indent_stack[0] = 0;
lexer->indent_count = 1;

}

Token lexer_next(Lexer *lexer)
{
if (lexer->pending_dedents > 0) {
lexer->pending_dedents--;

    return make_empty_token(
        lexer,
        TOKEN_DEDENT
    );
}

if (lexer->at_line_start)
    return handle_indentation(lexer);

while (
    *lexer->current == ' ' ||
    *lexer->current == '\t' ||
    *lexer->current == '\r'
) {
    advance_char(lexer);
}

const char *start = lexer->current;
int line = lexer->line;
int column = lexer->column;

int indent =
    lexer->indent_stack[
        lexer->indent_count - 1
    ];

if (*lexer->current == '\0') {
    if (lexer->indent_count > 1) {
        lexer->indent_count--;

        return make_empty_token(
            lexer,
            TOKEN_DEDENT
        );
    }

    lexer->finished = 1;

    return make_token(
        lexer,
        TOKEN_EOF,
        start,
        line,
        column,
        indent
    );
}

if (*lexer->current == '\n') {
    advance_char(lexer);

    return make_token(
        lexer,
        TOKEN_NEWLINE,
        start,
        line,
        column,
        indent
    );
}

if (
    isalpha(
        (unsigned char)*lexer->current
    ) ||
    *lexer->current == '_'
) {
    while (
        isalnum(
            (unsigned char)*lexer->current
        ) ||
        *lexer->current == '_'
    ) {
        advance_char(lexer);
    }

    int length =
        (int)(lexer->current - start);

    return make_token(
        lexer,
        keyword_type(start, length),
        start,
        line,
        column,
        indent
    );
}

if (
    isdigit(
        (unsigned char)*lexer->current
    ) ||
    (
        *lexer->current == '.' &&
        isdigit(
            (unsigned char)lexer->current[1]
        )
    )
) {
    while (
        isdigit(
            (unsigned char)*lexer->current
        )
    ) {
        advance_char(lexer);
    }

    if (*lexer->current == '.') {
        advance_char(lexer);

        while (
            isdigit(
                (unsigned char)*lexer->current
            )
        ) {
            advance_char(lexer);
        }
    }

    return make_token(
        lexer,
        TOKEN_NUMBER,
        start,
        line,
        column,
        indent
    );
}

if (*lexer->current == '"') {
    advance_char(lexer);

    while (
        *lexer->current != '\0' &&
        *lexer->current != '"'
    ) {
        if (
            *lexer->current == '\\' &&
            lexer->current[1] != '\0'
        ) {
            advance_char(lexer);
        }

        advance_char(lexer);
    }

    if (*lexer->current == '"')
        advance_char(lexer);

    return make_token(
        lexer,
        TOKEN_STRING,
        start,
        line,
        column,
        indent
    );
}

if (
    lexer->current[0] == '-' &&
    lexer->current[1] == '>'
) {
    advance_char(lexer);
    advance_char(lexer);

    return make_token(
        lexer,
        TOKEN_ARROW,
        start,
        line,
        column,
        indent
    );
}

if (
    lexer->current[0] == '=' &&
    lexer->current[1] == '='
) {
    advance_char(lexer);
    advance_char(lexer);

    return make_token(
        lexer,
        TOKEN_EQUAL_EQUAL,
        start,
        line,
        column,
        indent
    );
}

if (
    lexer->current[0] == '!' &&
    lexer->current[1] == '='
) {
    advance_char(lexer);
    advance_char(lexer);

    return make_token(
        lexer,
        TOKEN_NOT_EQUAL,
        start,
        line,
        column,
        indent
    );
}

if (
    lexer->current[0] == '<' &&
    lexer->current[1] == '='
) {
    advance_char(lexer);
    advance_char(lexer);

    return make_token(
        lexer,
        TOKEN_LESS_EQUAL,
        start,
        line,
        column,
        indent
    );
}

if (
    lexer->current[0] == '>' &&
    lexer->current[1] == '='
) {
    advance_char(lexer);
    advance_char(lexer);

    return make_token(
        lexer,
        TOKEN_GREATER_EQUAL,
        start,
        line,
        column,
        indent
    );
}

switch (*lexer->current) {
    case '+':
        advance_char(lexer);
        return make_token(
            lexer,
            TOKEN_PLUS,
            start,
            line,
            column,
            indent
        );

    case '-':
        advance_char(lexer);
        return make_token(
            lexer,
            TOKEN_MINUS,
            start,
            line,
            column,
            indent
        );

    case '*':
        advance_char(lexer);
        return make_token(
            lexer,
            TOKEN_STAR,
            start,
            line,
            column,
            indent
        );

    case '/':
        advance_char(lexer);
        return make_token(
            lexer,
            TOKEN_SLASH,
            start,
            line,
            column,
            indent
        );

    case '%':
        advance_char(lexer);
        return make_token(
            lexer,
            TOKEN_PERCENT,
            start,
            line,
            column,
            indent
        );

    case '<':
        advance_char(lexer);
        return make_token(
            lexer,
            TOKEN_LESS,
            start,
            line,
            column,
            indent
        );

    case '>':
        advance_char(lexer);
        return make_token(
            lexer,
            TOKEN_GREATER,
            start,
            line,
            column,
            indent
        );

    default:
        advance_char(lexer);

        return make_token(
            lexer,
            TOKEN_EOF,
            start,
            line,
            column,
            indent
        );
}

}

