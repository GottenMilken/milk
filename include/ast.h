#ifndef MILK_AST_H
#define MILK_AST_H

#include <stddef.h>

typedef enum {
AST_PROGRAM,
AST_ASSIGNMENT,
AST_SHOW,
AST_CHECK,
AST_REPEAT,
AST_DURING,
AST_FUNCTION,
AST_GIVE,
AST_CALL,
AST_NUMBER,
AST_STRING,
AST_BOOLEAN,
AST_EMPTY,
AST_VARIABLE,
AST_BINARY
} AstType;

typedef enum {
BINARY_ADD,
BINARY_SUBTRACT,
BINARY_MULTIPLY,
BINARY_DIVIDE,
BINARY_MODULO,
BINARY_EQUAL,
BINARY_NOT_EQUAL,
BINARY_LESS,
BINARY_LESS_EQUAL,
BINARY_GREATER,
BINARY_GREATER_EQUAL
} BinaryOperator;

typedef struct AstNode AstNode;

struct AstNode {
AstType type;
int line;
int column;

union {
    struct {
        AstNode **statements;
        size_t count;
        size_t capacity;
    } program;

    struct {
        char *name;
        AstNode *value;
    } assignment;

    struct {
        AstNode *value;
    } show;

    struct {
        AstNode *condition;
        AstNode **body;
        size_t body_count;
        size_t body_capacity;
        AstNode *otherwise;
    } check;

    struct {
        AstNode *times;
        AstNode **body;
        size_t body_count;
        size_t body_capacity;
    } repeat;

    struct {
        AstNode *condition;
        AstNode **body;
        size_t body_count;
        size_t body_capacity;
    } during;

    struct {
        char *name;
        char **parameters;
        size_t parameter_count;
        size_t parameter_capacity;
        AstNode *body;
    } function;

    struct {
        AstNode *value;
    } give;

    struct {
        char *name;
        AstNode **arguments;
        size_t argument_count;
        size_t argument_capacity;
    } call;

    struct {
        double value;
    } number;

    struct {
        char *value;
    } string;

    struct {
        int value;
    } boolean;

    struct {
        char *name;
    } variable;

    struct {
        BinaryOperator operator;
        AstNode *left;
        AstNode *right;
    } binary;
};
};

AstNode *ast_program(void);
AstNode *ast_assignment(const char *name, AstNode *value);
AstNode *ast_show(AstNode *value);
AstNode *ast_check(AstNode *condition);
AstNode *ast_repeat(AstNode *times);
AstNode *ast_during(AstNode *condition);
AstNode *ast_function(const char *name);
AstNode *ast_give(AstNode *value);
AstNode *ast_call(const char *name);
AstNode *ast_number(double value);
AstNode *ast_string(const char *value);
AstNode *ast_boolean(int value);
AstNode *ast_empty(void);
AstNode *ast_variable(const char *name);

AstNode *ast_binary(
BinaryOperator operator,
AstNode *left,
AstNode *right
);

void ast_program_add(
AstNode *program,
AstNode *statement
);

void ast_check_add(
AstNode *check,
AstNode *statement
);

void ast_repeat_add(
AstNode *repeat,
AstNode *statement
);

void ast_during_add(
AstNode *during,
AstNode *statement
);

void ast_function_add_parameter(
AstNode *function,
const char *parameter
);

void ast_function_set_body(
AstNode *function,
AstNode *body
);

void ast_call_add_argument(
AstNode *call,
AstNode *argument
);

void ast_free(AstNode *node);
void ast_print(const AstNode *node, int indent);

#endif
