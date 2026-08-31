#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void advance_parser(Parser *parser)
{
    parser->previous =
        parser->current;

    parser->current =
        lexer_next(&parser->lexer);
}

static int check_token(
    Parser *parser,
    TokenType type
)
{
    return parser->current.type == type;
}

static int match_token(
    Parser *parser,
    TokenType type
)
{
    if (!check_token(parser, type))
        return 0;

    advance_parser(parser);

    return 1;
}

static void skip_newlines(Parser *parser)
{
    while (
        match_token(
            parser,
            TOKEN_NEWLINE
        )
    )
        ;
}

static char *token_string(Token token)
{
    char *result =
        malloc(token.length + 1);

    if (!result)
        return NULL;

    memcpy(
        result,
        token.start,
        token.length
    );

    result[token.length] = '\0';

    return result;
}

static void parser_error(
    Parser *parser,
    const char *message
)
{
    fprintf(
        stderr,
        "Milk error at %d:%d: %s\n",
        parser->current.line,
        parser->current.column,
        message
    );

    parser->had_error = 1;
}

static AstNode *parse_expression(
    Parser *parser
);

static AstNode *parse_statement(
    Parser *parser
);

static AstNode *parse_primary(
    Parser *parser
)
{
    if (match_token(
        parser,
        TOKEN_NUMBER
    )) {
        char *text =
            token_string(parser->previous);

        if (!text)
            return NULL;

        double value =
            strtod(text, NULL);

        free(text);

        return ast_number(value);
    }

    if (match_token(
        parser,
        TOKEN_STRING
    )) {
        Token token =
            parser->previous;

        if (token.length < 2)
            return ast_string("");

        char *value =
            malloc(token.length - 1);

        if (!value)
            return NULL;

        memcpy(
            value,
            token.start + 1,
            token.length - 2
        );

        value[token.length - 2] =
            '\0';

        AstNode *node =
            ast_string(value);

        free(value);

        return node;
    }

    if (match_token(
        parser,
        TOKEN_YES
    ))
        return ast_boolean(1);

    if (match_token(
        parser,
        TOKEN_NO
    ))
        return ast_boolean(0);

    if (match_token(
        parser,
        TOKEN_EMPTY
    ))
        return ast_empty();

    if (match_token(
        parser,
        TOKEN_IDENTIFIER
    )) {
        char *name =
            token_string(parser->previous);

        if (!name)
            return NULL;

        AstNode *node =
            ast_variable(name);

        free(name);

        return node;
    }

    parser_error(
        parser,
        "expected a value"
    );

    return NULL;
}

static AstNode *parse_multiplication(
    Parser *parser
)
{
    AstNode *left =
        parse_primary(parser);

    if (!left)
        return NULL;

    for (;;) {
        BinaryOperator operator;

        if (match_token(
            parser,
            TOKEN_STAR
        )) {
            operator =
                BINARY_MULTIPLY;
        } else if (
            match_token(
                parser,
                TOKEN_SLASH
            )
        ) {
            operator =
                BINARY_DIVIDE;
        } else if (
            match_token(
                parser,
                TOKEN_PERCENT
            )
        ) {
            operator =
                BINARY_MODULO;
        } else {
            break;
        }

        AstNode *right =
            parse_primary(parser);

        if (!right) {
            ast_free(left);
            return NULL;
        }

        left =
            ast_binary(
                operator,
                left,
                right
            );
    }

    return left;
}

static AstNode *parse_addition(
    Parser *parser
)
{
    AstNode *left =
        parse_multiplication(parser);

    if (!left)
        return NULL;

    for (;;) {
        BinaryOperator operator;

        if (match_token(
            parser,
            TOKEN_PLUS
        )) {
            operator = BINARY_ADD;
        } else if (
            match_token(
                parser,
                TOKEN_MINUS
            )
        ) {
            operator =
                BINARY_SUBTRACT;
        } else {
            break;
        }

        AstNode *right =
            parse_multiplication(parser);

        if (!right) {
            ast_free(left);
            return NULL;
        }

        left =
            ast_binary(
                operator,
                left,
                right
            );
    }

    return left;
}

static AstNode *parse_comparison(
    Parser *parser
)
{
    AstNode *left =
        parse_addition(parser);

    if (!left)
        return NULL;

    for (;;) {
        BinaryOperator operator;

        if (match_token(
            parser,
            TOKEN_EQUAL_EQUAL
        )) {
            operator = BINARY_EQUAL;
        } else if (
            match_token(
                parser,
                TOKEN_NOT_EQUAL
            )
        ) {
            operator =
                BINARY_NOT_EQUAL;
        } else if (
            match_token(
                parser,
                TOKEN_LESS_EQUAL
            )
        ) {
            operator =
                BINARY_LESS_EQUAL;
        } else if (
            match_token(
                parser,
                TOKEN_LESS
            )
        ) {
            operator =
                BINARY_LESS;
        } else if (
            match_token(
                parser,
                TOKEN_GREATER_EQUAL
            )
        ) {
            operator =
                BINARY_GREATER_EQUAL;
        } else if (
            match_token(
                parser,
                TOKEN_GREATER
            )
        ) {
            operator =
                BINARY_GREATER;
        } else {
            break;
        }

        AstNode *right =
            parse_addition(parser);

        if (!right) {
            ast_free(left);
            return NULL;
        }

        left =
            ast_binary(
                operator,
                left,
                right
            );
    }

    return left;
}

static AstNode *parse_expression(
    Parser *parser
)
{
    return parse_comparison(parser);
}

static AstNode *parse_assignment(
    Parser *parser
)
{
    Token name_token =
        parser->current;

    advance_parser(parser);

    if (!match_token(
        parser,
        TOKEN_ARROW
    )) {
        parser_error(
            parser,
            "expected '->'"
        );

        return NULL;
    }

    AstNode *value =
        parse_expression(parser);

    if (!value)
        return NULL;

    char *name =
        token_string(name_token);

    if (!name) {
        ast_free(value);
        return NULL;
    }

    AstNode *node =
        ast_assignment(
            name,
            value
        );

    free(name);

    return node;
}

static AstNode *parse_show(
    Parser *parser
)
{
    advance_parser(parser);

    AstNode *value =
        parse_expression(parser);

    if (!value)
        return NULL;

    return ast_show(value);
}

static int begin_block(Parser *parser)
{
    if (!match_token(
        parser,
        TOKEN_NEWLINE
    )) {
        parser_error(
            parser,
            "expected a new line"
        );

        return 0;
    }

    if (!match_token(
        parser,
        TOKEN_INDENT
    )) {
        parser_error(
            parser,
            "expected an indented block"
        );

        return 0;
    }

    return 1;
}

static AstNode *parse_check(
    Parser *parser
)
{
    advance_parser(parser);

    AstNode *condition =
        parse_expression(parser);

    if (!condition)
        return NULL;

    AstNode *node =
        ast_check(condition);

    if (!node)
        return NULL;

    if (!begin_block(parser)) {
        ast_free(node);
        return NULL;
    }

    while (
        !check_token(
            parser,
            TOKEN_DEDENT
        ) &&
        !check_token(
            parser,
            TOKEN_EOF
        )
    ) {
        if (match_token(
            parser,
            TOKEN_NEWLINE
        ))
            continue;

        AstNode *statement =
            parse_statement(parser);

        if (!statement) {
            ast_free(node);
            return NULL;
        }

        ast_check_add(
            node,
            statement
        );

        if (
            check_token(
                parser,
                TOKEN_NEWLINE
            )
        ) {
            skip_newlines(parser);
        }
    }

    if (!match_token(
        parser,
        TOKEN_DEDENT
    )) {
        parser_error(
            parser,
            "expected end of block"
        );

        ast_free(node);
        return NULL;
    }

    if (match_token(
        parser,
        TOKEN_OTHERWISE
    )) {
        if (check_token(
            parser,
            TOKEN_CHECK
        )) {
            AstNode *otherwise =
                parse_check(parser);

            if (!otherwise) {
                ast_free(node);
                return NULL;
            }

            node->check.otherwise =
                otherwise;
        } else {
            if (!begin_block(parser)) {
                ast_free(node);
                return NULL;
            }

            AstNode *otherwise =
                ast_program();

            if (!otherwise) {
                ast_free(node);
                return NULL;
            }

            while (
                !check_token(
                    parser,
                    TOKEN_DEDENT
                ) &&
                !check_token(
                    parser,
                    TOKEN_EOF
                )
            ) {
                if (match_token(
                    parser,
                    TOKEN_NEWLINE
                ))
                    continue;

                AstNode *statement =
                    parse_statement(parser);

                if (!statement) {
                    ast_free(otherwise);
                    ast_free(node);
                    return NULL;
                }

                ast_program_add(
                    otherwise,
                    statement
                );

                if (
                    check_token(
                        parser,
                        TOKEN_NEWLINE
                    )
                ) {
                    skip_newlines(parser);
                }
            }

            if (!match_token(
                parser,
                TOKEN_DEDENT
            )) {
                parser_error(
                    parser,
                    "expected end of block"
                );

                ast_free(otherwise);
                ast_free(node);
                return NULL;
            }

            node->check.otherwise =
                otherwise;
        }
    }

    return node;
}

static AstNode *parse_repeat(
    Parser *parser
)
{
    advance_parser(parser);

    AstNode *times =
        parse_expression(parser);

    if (!times)
        return NULL;

    AstNode *node =
        ast_repeat(times);

    if (!node)
        return NULL;

    if (!begin_block(parser)) {
        ast_free(node);
        return NULL;
    }

    while (
        !check_token(
            parser,
            TOKEN_DEDENT
        ) &&
        !check_token(
            parser,
            TOKEN_EOF
        )
    ) {
        if (match_token(
            parser,
            TOKEN_NEWLINE
        ))
            continue;

        AstNode *statement =
            parse_statement(parser);

        if (!statement) {
            ast_free(node);
            return NULL;
        }

        ast_repeat_add(
            node,
            statement
        );

        if (
            check_token(
                parser,
                TOKEN_NEWLINE
            )
        ) {
            skip_newlines(parser);
        }
    }

    if (!match_token(
        parser,
        TOKEN_DEDENT
    )) {
        parser_error(
            parser,
            "expected end of block"
        );

        ast_free(node);
        return NULL;
    }

    return node;
}

static AstNode *parse_statement(
    Parser *parser
)
{
    if (check_token(
        parser,
        TOKEN_IDENTIFIER
    ))
        return parse_assignment(parser);

    if (check_token(
        parser,
        TOKEN_SHOW
    ))
        return parse_show(parser);

    if (check_token(
        parser,
        TOKEN_CHECK
    ))
        return parse_check(parser);

    if (check_token(
        parser,
        TOKEN_REPEAT
    ))
        return parse_repeat(parser);

    parser_error(
        parser,
        "expected a statement"
    );

    return NULL;
}

void parser_init(
    Parser *parser,
    const char *source
)
{
    memset(
        parser,
        0,
        sizeof(Parser)
    );

    lexer_init(
        &parser->lexer,
        source
    );

    parser->current =
        lexer_next(&parser->lexer);
}

AstNode *parser_parse(
    Parser *parser
)
{
    AstNode *program =
        ast_program();

    if (!program)
        return NULL;

    skip_newlines(parser);

    while (
        !check_token(
            parser,
            TOKEN_EOF
        )
    ) {
        if (match_token(
            parser,
            TOKEN_NEWLINE
        ))
            continue;

        AstNode *statement =
            parse_statement(parser);

        if (!statement) {
            ast_free(program);
            return NULL;
        }

        ast_program_add(
            program,
            statement
        );

        if (
            check_token(
                parser,
                TOKEN_NEWLINE
            )
        ) {
            skip_newlines(parser);
        }
    }

    if (parser->had_error) {
        ast_free(program);
        return NULL;
    }

    return program;
}
