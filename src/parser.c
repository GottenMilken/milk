#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void advance_parser(Parser *parser)
{
    parser->previous = parser->current;
    parser->current = lexer_next(&parser->lexer);
}

static int check_token(Parser *parser, TokenType type)
{
    return parser->current.type == type;
}

static int match_token(Parser *parser, TokenType type)
{
    if (!check_token(parser, type))
        return 0;

    advance_parser(parser);
    return 1;
}

static void skip_newlines(Parser *parser)
{
    while (match_token(parser, TOKEN_NEWLINE))
        ;
}

static char *token_string(Token token)
{
    char *result = malloc((size_t)token.length + 1);

    if (!result)
        return NULL;

    memcpy(result, token.start, (size_t)token.length);
    result[token.length] = '\0';
    return result;
}

static void parser_error(Parser *parser, const char *message)
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

static AstNode *parse_expression(Parser *parser);
static AstNode *parse_run_expression(Parser *parser);
static AstNode *parse_statement(Parser *parser);

static AstNode *parse_call_after_name(
    Parser *parser,
    const char *name
)
{
    AstNode *call = ast_call(name);

    if (!call)
        return NULL;

    if (!match_token(parser, TOKEN_LEFT_PAREN)) {
        ast_free(call);
        parser_error(parser, "expected '('");
        return NULL;
    }

    if (match_token(parser, TOKEN_RIGHT_PAREN))
        return call;

    for (;;) {
        AstNode *argument = parse_expression(parser);

        if (!argument) {
            ast_free(call);
            return NULL;
        }

        ast_call_add_argument(call, argument);

        if (match_token(parser, TOKEN_COMMA))
            continue;

        if (!match_token(parser, TOKEN_RIGHT_PAREN)) {
            ast_free(call);
            parser_error(parser, "expected ',' or ')' in function call");
            return NULL;
        }

        break;
    }

    return call;
}

static AstNode *parse_list(Parser *parser)
{
    advance_parser(parser);

    AstNode *list = ast_list();

    if (!list)
        return NULL;

    if (match_token(parser, TOKEN_RIGHT_BRACKET))
        return list;

    for (;;) {
        AstNode *item = parse_expression(parser);

        if (!item) {
            ast_free(list);
            return NULL;
        }

        ast_list_add(list, item);

        if (match_token(parser, TOKEN_COMMA))
            continue;

        if (!match_token(parser, TOKEN_RIGHT_BRACKET)) {
            ast_free(list);
            parser_error(parser, "expected ',' or ']' in list");
            return NULL;
        }

        break;
    }

    return list;
}

static AstNode *parse_primary(Parser *parser)
{
    if (match_token(parser, TOKEN_NUMBER)) {
        char *text = token_string(parser->previous);

        if (!text)
            return NULL;

        double value = strtod(text, NULL);
        free(text);
        return ast_number(value);
    }

    if (match_token(parser, TOKEN_STRING)) {
        Token token = parser->previous;

        if (token.length < 2)
            return ast_string("");

        char *value = malloc((size_t)token.length - 1);

        if (!value)
            return NULL;

        memcpy(
            value,
            token.start + 1,
            (size_t)token.length - 2
        );
        value[token.length - 2] = '\0';

        AstNode *node = ast_string(value);
        free(value);
        return node;
    }

    if (match_token(parser, TOKEN_YES))
        return ast_boolean(1);

    if (match_token(parser, TOKEN_NO))
        return ast_boolean(0);

    if (match_token(parser, TOKEN_EMPTY))
        return ast_empty();

    if (check_token(parser, TOKEN_LEFT_BRACKET))
        return parse_list(parser);

    if (check_token(parser, TOKEN_RUN))
        return parse_run_expression(parser);

    if (match_token(parser, TOKEN_IDENTIFIER)) {
        char *name = token_string(parser->previous);

        if (!name)
            return NULL;

        AstNode *node;

        if (check_token(parser, TOKEN_LEFT_PAREN))
            node = parse_call_after_name(parser, name);
        else
            node = ast_variable(name);

        free(name);
        return node;
    }

    parser_error(parser, "expected a value");
    return NULL;
}

static AstNode *parse_multiplication(Parser *parser)
{
    AstNode *left = parse_primary(parser);

    if (!left)
        return NULL;

    for (;;) {
        BinaryOperator operator;

        if (match_token(parser, TOKEN_STAR))
            operator = BINARY_MULTIPLY;
        else if (match_token(parser, TOKEN_SLASH))
            operator = BINARY_DIVIDE;
        else if (match_token(parser, TOKEN_PERCENT))
            operator = BINARY_MODULO;
        else
            break;

        AstNode *right = parse_primary(parser);

        if (!right) {
            ast_free(left);
            return NULL;
        }

        left = ast_binary(operator, left, right);

        if (!left)
            return NULL;
    }

    return left;
}

static AstNode *parse_addition(Parser *parser)
{
    AstNode *left = parse_multiplication(parser);

    if (!left)
        return NULL;

    for (;;) {
        BinaryOperator operator;

        if (match_token(parser, TOKEN_PLUS))
            operator = BINARY_ADD;
        else if (match_token(parser, TOKEN_MINUS))
            operator = BINARY_SUBTRACT;
        else
            break;

        AstNode *right = parse_multiplication(parser);

        if (!right) {
            ast_free(left);
            return NULL;
        }

        left = ast_binary(operator, left, right);

        if (!left)
            return NULL;
    }

    return left;
}

static AstNode *parse_comparison(Parser *parser)
{
    AstNode *left = parse_addition(parser);

    if (!left)
        return NULL;

    for (;;) {
        BinaryOperator operator;

        if (match_token(parser, TOKEN_EQUAL_EQUAL))
            operator = BINARY_EQUAL;
        else if (match_token(parser, TOKEN_NOT_EQUAL))
            operator = BINARY_NOT_EQUAL;
        else if (match_token(parser, TOKEN_LESS_EQUAL))
            operator = BINARY_LESS_EQUAL;
        else if (match_token(parser, TOKEN_LESS))
            operator = BINARY_LESS;
        else if (match_token(parser, TOKEN_GREATER_EQUAL))
            operator = BINARY_GREATER_EQUAL;
        else if (match_token(parser, TOKEN_GREATER))
            operator = BINARY_GREATER;
        else
            break;

        AstNode *right = parse_addition(parser);

        if (!right) {
            ast_free(left);
            return NULL;
        }

        left = ast_binary(operator, left, right);

        if (!left)
            return NULL;
    }

    return left;
}

static AstNode *parse_expression(Parser *parser)
{
    return parse_comparison(parser);
}

static AstNode *parse_show(Parser *parser)
{
    advance_parser(parser);

    AstNode *value = parse_expression(parser);

    if (!value)
        return NULL;

    return ast_show(value);
}

static AstNode *parse_run_expression(Parser *parser)
{
    advance_parser(parser);

    AstNode *command = parse_expression(parser);

    if (!command)
        return NULL;

    return ast_run(command);
}

static int begin_block(Parser *parser)
{
    if (check_token(parser, TOKEN_NEWLINE))
        skip_newlines(parser);

    if (!match_token(parser, TOKEN_INDENT)) {
        parser_error(parser, "expected an indented block");
        return 0;
    }

    return 1;
}

static AstNode *parse_block_program(Parser *parser)
{
    if (!begin_block(parser))
        return NULL;

    AstNode *program = ast_program();

    if (!program)
        return NULL;

    while (
        !check_token(parser, TOKEN_DEDENT) &&
        !check_token(parser, TOKEN_EOF)
    ) {
        if (match_token(parser, TOKEN_NEWLINE))
            continue;

        AstNode *statement = parse_statement(parser);

        if (!statement) {
            ast_free(program);
            return NULL;
        }

        ast_program_add(program, statement);

        if (check_token(parser, TOKEN_NEWLINE))
            skip_newlines(parser);
    }

    if (!match_token(parser, TOKEN_DEDENT)) {
        parser_error(parser, "expected end of block");
        ast_free(program);
        return NULL;
    }

    return program;
}

static AstNode *parse_check(Parser *parser)
{
    advance_parser(parser);

    AstNode *condition = parse_expression(parser);

    if (!condition)
        return NULL;

    AstNode *node = ast_check(condition);

    if (!node)
        return NULL;

    AstNode *body = parse_block_program(parser);

    if (!body) {
        ast_free(node);
        return NULL;
    }

    node->check.body = body->program.statements;
    node->check.body_count = body->program.count;
    node->check.body_capacity = body->program.capacity;
    body->program.statements = NULL;
    body->program.count = 0;
    body->program.capacity = 0;
    ast_free(body);

    if (match_token(parser, TOKEN_OTHERWISE)) {
        if (check_token(parser, TOKEN_CHECK)) {
            AstNode *otherwise = parse_check(parser);

            if (!otherwise) {
                ast_free(node);
                return NULL;
            }

            node->check.otherwise = otherwise;
        } else {
            AstNode *otherwise = parse_block_program(parser);

            if (!otherwise) {
                ast_free(node);
                return NULL;
            }

            node->check.otherwise = otherwise;
        }
    }

    return node;
}

static AstNode *parse_repeat(Parser *parser)
{
    advance_parser(parser);

    AstNode *times = parse_expression(parser);

    if (!times)
        return NULL;

    AstNode *node = ast_repeat(times);

    if (!node)
        return NULL;

    AstNode *body = parse_block_program(parser);

    if (!body) {
        ast_free(node);
        return NULL;
    }

    node->repeat.body = body->program.statements;
    node->repeat.body_count = body->program.count;
    node->repeat.body_capacity = body->program.capacity;
    body->program.statements = NULL;
    body->program.count = 0;
    body->program.capacity = 0;
    ast_free(body);

    return node;
}

static AstNode *parse_during(Parser *parser)
{
    advance_parser(parser);

    AstNode *condition = parse_expression(parser);

    if (!condition)
        return NULL;

    AstNode *node = ast_during(condition);

    if (!node)
        return NULL;

    AstNode *body = parse_block_program(parser);

    if (!body) {
        ast_free(node);
        return NULL;
    }

    node->during.body = body->program.statements;
    node->during.body_count = body->program.count;
    node->during.body_capacity = body->program.capacity;
    body->program.statements = NULL;
    body->program.count = 0;
    body->program.capacity = 0;
    ast_free(body);

    return node;
}

static AstNode *parse_make(Parser *parser)
{
    advance_parser(parser);

    if (!check_token(parser, TOKEN_IDENTIFIER)) {
        parser_error(parser, "expected a function name");
        return NULL;
    }

    Token name_token = parser->current;
    advance_parser(parser);

    char *name = token_string(name_token);

    if (!name)
        return NULL;

    AstNode *function = ast_function(name);
    free(name);

    if (!function)
        return NULL;

    if (!match_token(parser, TOKEN_LEFT_PAREN)) {
        parser_error(parser, "expected '(' after function name");
        ast_free(function);
        return NULL;
    }

    if (!match_token(parser, TOKEN_RIGHT_PAREN)) {
        for (;;) {
            if (!check_token(parser, TOKEN_IDENTIFIER)) {
                parser_error(parser, "expected a parameter name");
                ast_free(function);
                return NULL;
            }

            char *parameter = token_string(parser->current);
            advance_parser(parser);

            if (!parameter) {
                ast_free(function);
                return NULL;
            }

            ast_function_add_parameter(function, parameter);
            free(parameter);

            if (match_token(parser, TOKEN_COMMA))
                continue;

            if (!match_token(parser, TOKEN_RIGHT_PAREN)) {
                parser_error(parser, "expected ',' or ')' in parameter list");
                ast_free(function);
                return NULL;
            }

            break;
        }
    }

    AstNode *body = parse_block_program(parser);

    if (!body) {
        ast_free(function);
        return NULL;
    }

    ast_function_set_body(function, body);
    return function;
}

static AstNode *parse_give(Parser *parser)
{
    advance_parser(parser);

    AstNode *value = parse_expression(parser);

    if (!value)
        return NULL;

    return ast_give(value);
}

static AstNode *parse_identifier_statement(Parser *parser)
{
    Token name_token = parser->current;
    advance_parser(parser);

    char *name = token_string(name_token);

    if (!name)
        return NULL;

    if (match_token(parser, TOKEN_ARROW)) {
        AstNode *value = parse_expression(parser);

        if (!value) {
            free(name);
            return NULL;
        }

        AstNode *node = ast_assignment(name, value);
        free(name);
        return node;
    }

    if (check_token(parser, TOKEN_LEFT_PAREN)) {
        AstNode *node = parse_call_after_name(parser, name);
        free(name);
        return node;
    }

    free(name);
    parser_error(parser, "expected '->' or a function call");
    return NULL;
}

static AstNode *parse_statement(Parser *parser)
{
    if (check_token(parser, TOKEN_IDENTIFIER))
        return parse_identifier_statement(parser);

    if (check_token(parser, TOKEN_SHOW))
        return parse_show(parser);

    if (check_token(parser, TOKEN_RUN))
        return parse_expression(parser);

    if (check_token(parser, TOKEN_CHECK))
        return parse_check(parser);

    if (check_token(parser, TOKEN_REPEAT))
        return parse_repeat(parser);

    if (check_token(parser, TOKEN_DURING))
        return parse_during(parser);

    if (check_token(parser, TOKEN_MAKE))
        return parse_make(parser);

    if (check_token(parser, TOKEN_GIVE))
        return parse_give(parser);

    parser_error(parser, "expected a statement");
    return NULL;
}

void parser_init(Parser *parser, const char *source)
{
    memset(parser, 0, sizeof(Parser));
    lexer_init(&parser->lexer, source);
    parser->current = lexer_next(&parser->lexer);
}

AstNode *parser_parse(Parser *parser)
{
    AstNode *program = ast_program();

    if (!program)
        return NULL;

    skip_newlines(parser);

    while (!check_token(parser, TOKEN_EOF)) {
        if (match_token(parser, TOKEN_NEWLINE))
            continue;

        AstNode *statement = parse_statement(parser);

        if (!statement) {
            ast_free(program);
            return NULL;
        }

        ast_program_add(program, statement);

        if (check_token(parser, TOKEN_NEWLINE))
            skip_newlines(parser);
    }

    if (parser->had_error) {
        ast_free(program);
        return NULL;
    }

    return program;
}
