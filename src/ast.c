#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *copy_string(const char *string)
{
    if (!string)
        return NULL;

    size_t length = strlen(string);

    char *copy = malloc(length + 1);

    if (!copy)
        return NULL;

    memcpy(copy, string, length + 1);

    return copy;
}

static AstNode *create_node(AstType type)
{
    AstNode *node = calloc(1, sizeof(AstNode));

    if (!node)
        return NULL;

    node->type = type;

    return node;
}

AstNode *ast_program(void)
{
    return create_node(AST_PROGRAM);
}

AstNode *ast_assignment(
    const char *name,
    AstNode *value
)
{
    AstNode *node =
        create_node(AST_ASSIGNMENT);

    if (!node)
        return NULL;

    node->assignment.name =
        copy_string(name);

    node->assignment.value =
        value;

    return node;
}

AstNode *ast_show(AstNode *value)
{
    AstNode *node =
        create_node(AST_SHOW);

    if (!node)
        return NULL;

    node->show.value = value;

    return node;
}

AstNode *ast_check(AstNode *condition)
{
    AstNode *node =
        create_node(AST_CHECK);

    if (!node)
        return NULL;

    node->check.condition = condition;

    return node;
}

AstNode *ast_repeat(AstNode *times)
{
    AstNode *node =
        create_node(AST_REPEAT);

    if (!node)
        return NULL;

    node->repeat.times = times;

    return node;
}

AstNode *ast_during(AstNode *condition)
{
    AstNode *node =
        create_node(AST_DURING);

    if (!node)
        return NULL;

    node->during.condition = condition;

    return node;
}

AstNode *ast_number(double value)
{
    AstNode *node =
        create_node(AST_NUMBER);

    if (!node)
        return NULL;

    node->number.value = value;

    return node;
}

AstNode *ast_string(const char *value)
{
    AstNode *node =
        create_node(AST_STRING);

    if (!node)
        return NULL;

    node->string.value =
        copy_string(value);

    return node;
}

AstNode *ast_boolean(int value)
{
    AstNode *node =
        create_node(AST_BOOLEAN);

    if (!node)
        return NULL;

    node->boolean.value =
        value ? 1 : 0;

    return node;
}

AstNode *ast_empty(void)
{
    return create_node(AST_EMPTY);
}

AstNode *ast_variable(const char *name)
{
    AstNode *node =
        create_node(AST_VARIABLE);

    if (!node)
        return NULL;

    node->variable.name =
        copy_string(name);

    return node;
}

AstNode *ast_binary(
    BinaryOperator operator,
    AstNode *left,
    AstNode *right
)
{
    AstNode *node =
        create_node(AST_BINARY);

    if (!node)
        return NULL;

    node->binary.operator = operator;
    node->binary.left = left;
    node->binary.right = right;

    return node;
}

void ast_program_add(
    AstNode *program,
    AstNode *statement
)
{
    if (!program ||
        program->type != AST_PROGRAM ||
        !statement)
        return;

    if (
        program->program.count >=
        program->program.capacity
    ) {
        size_t capacity =
            program->program.capacity == 0
                ? 4
                : program->program.capacity * 2;

        AstNode **statements =
            realloc(
                program->program.statements,
                sizeof(AstNode *) * capacity
            );

        if (!statements)
            return;

        program->program.statements =
            statements;

        program->program.capacity =
            capacity;
    }

    program->program.statements[
        program->program.count++
    ] = statement;
}

void ast_check_add(
    AstNode *check,
    AstNode *statement
)
{
    if (!check ||
        check->type != AST_CHECK ||
        !statement)
        return;

    if (
        check->check.body_count >=
        check->check.body_capacity
    ) {
        size_t capacity =
            check->check.body_capacity == 0
                ? 4
                : check->check.body_capacity * 2;

        AstNode **body =
            realloc(
                check->check.body,
                sizeof(AstNode *) * capacity
            );

        if (!body)
            return;

        check->check.body = body;

        check->check.body_capacity =
            capacity;
    }

    check->check.body[
        check->check.body_count++
    ] = statement;
}

void ast_repeat_add(
    AstNode *repeat,
    AstNode *statement
)
{
    if (!repeat ||
        repeat->type != AST_REPEAT ||
        !statement)
        return;

    if (
        repeat->repeat.body_count >=
        repeat->repeat.body_capacity
    ) {
        size_t capacity =
            repeat->repeat.body_capacity == 0
                ? 4
                : repeat->repeat.body_capacity * 2;

        AstNode **body =
            realloc(
                repeat->repeat.body,
                sizeof(AstNode *) * capacity
            );

        if (!body)
            return;

        repeat->repeat.body = body;

        repeat->repeat.body_capacity =
            capacity;
    }

    repeat->repeat.body[
        repeat->repeat.body_count++
    ] = statement;
}

void ast_during_add(
    AstNode *during,
    AstNode *statement
)
{
    if (!during ||
        during->type != AST_DURING ||
        !statement)
        return;

    if (
        during->during.body_count >=
        during->during.body_capacity
    ) {
        size_t capacity =
            during->during.body_capacity == 0
                ? 4
                : during->during.body_capacity * 2;

        AstNode **body =
            realloc(
                during->during.body,
                sizeof(AstNode *) * capacity
            );

        if (!body)
            return;

        during->during.body = body;

        during->during.body_capacity =
            capacity;
    }

    during->during.body[
        during->during.body_count++
    ] = statement;
}

static void print_indent(int indent)
{
    for (int i = 0; i < indent; i++)
        printf("  ");
}

static const char *binary_name(
    BinaryOperator operator
)
{
    switch (operator) {
        case BINARY_ADD:
            return "Add";

        case BINARY_SUBTRACT:
            return "Subtract";

        case BINARY_MULTIPLY:
            return "Multiply";

        case BINARY_DIVIDE:
            return "Divide";

        case BINARY_MODULO:
            return "Modulo";

        case BINARY_EQUAL:
            return "Equal";

        case BINARY_NOT_EQUAL:
            return "Not Equal";

        case BINARY_LESS:
            return "Less";

        case BINARY_LESS_EQUAL:
            return "Less Or Equal";

        case BINARY_GREATER:
            return "Greater";

        case BINARY_GREATER_EQUAL:
            return "Greater Or Equal";
    }

    return "Unknown";
}

void ast_print(
    const AstNode *node,
    int indent
)
{
    if (!node)
        return;

    switch (node->type) {
        case AST_PROGRAM:
            print_indent(indent);
            printf("Program\n");

            for (
                size_t i = 0;
                i < node->program.count;
                i++
            ) {
                ast_print(
                    node->program.statements[i],
                    indent + 1
                );
            }

            break;

        case AST_ASSIGNMENT:
            print_indent(indent);
            printf(
                "Assignment: %s\n",
                node->assignment.name
            );

            ast_print(
                node->assignment.value,
                indent + 1
            );

            break;

        case AST_SHOW:
            print_indent(indent);
            printf("Show\n");

            ast_print(
                node->show.value,
                indent + 1
            );

            break;

        case AST_CHECK:
            print_indent(indent);
            printf("Check\n");

            print_indent(indent + 1);
            printf("Condition\n");

            ast_print(
                node->check.condition,
                indent + 2
            );

            print_indent(indent + 1);
            printf("Body\n");

            for (
                size_t i = 0;
                i < node->check.body_count;
                i++
            ) {
                ast_print(
                    node->check.body[i],
                    indent + 2
                );
            }

            if (node->check.otherwise) {
                print_indent(indent + 1);
                printf("Otherwise\n");

                ast_print(
                    node->check.otherwise,
                    indent + 2
                );
            }

            break;

        case AST_REPEAT:
            print_indent(indent);
            printf("Repeat\n");

            print_indent(indent + 1);
            printf("Times\n");

            ast_print(
                node->repeat.times,
                indent + 2
            );

            print_indent(indent + 1);
            printf("Body\n");

            for (
                size_t i = 0;
                i < node->repeat.body_count;
                i++
            ) {
                ast_print(
                    node->repeat.body[i],
                    indent + 2
                );
            }

            break;

        case AST_DURING:
            print_indent(indent);
            printf("During\n");

            print_indent(indent + 1);
            printf("Condition\n");

            ast_print(
                node->during.condition,
                indent + 2
            );

            print_indent(indent + 1);
            printf("Body\n");

            for (
                size_t i = 0;
                i < node->during.body_count;
                i++
            ) {
                ast_print(
                    node->during.body[i],
                    indent + 2
                );
            }

            break;

        case AST_NUMBER:
            print_indent(indent);
            printf(
                "Number: %g\n",
                node->number.value
            );

            break;

        case AST_STRING:
            print_indent(indent);
            printf(
                "String: \"%s\"\n",
                node->string.value
            );

            break;

        case AST_BOOLEAN:
            print_indent(indent);
            printf(
                "Boolean: %s\n",
                node->boolean.value
                    ? "yes"
                    : "no"
            );

            break;

        case AST_EMPTY:
            print_indent(indent);
            printf("Empty\n");

            break;

        case AST_VARIABLE:
            print_indent(indent);
            printf(
                "Variable: %s\n",
                node->variable.name
            );

            break;

        case AST_BINARY:
            print_indent(indent);
            printf(
                "Binary: %s\n",
                binary_name(
                    node->binary.operator
                )
            );

            ast_print(
                node->binary.left,
                indent + 1
            );

            ast_print(
                node->binary.right,
                indent + 1
            );

            break;
    }
}

void ast_free(AstNode *node)
{
    if (!node)
        return;

    switch (node->type) {
        case AST_PROGRAM:
            for (
                size_t i = 0;
                i < node->program.count;
                i++
            ) {
                ast_free(
                    node->program.statements[i]
                );
            }

            free(
                node->program.statements
            );

            break;

        case AST_ASSIGNMENT:
            free(node->assignment.name);
            ast_free(node->assignment.value);

            break;

        case AST_SHOW:
            ast_free(node->show.value);

            break;

        case AST_CHECK:
            ast_free(node->check.condition);

            for (
                size_t i = 0;
                i < node->check.body_count;
                i++
            ) {
                ast_free(
                    node->check.body[i]
                );
            }

            free(node->check.body);

            ast_free(node->check.otherwise);

            break;

        case AST_REPEAT:
            ast_free(node->repeat.times);

            for (
                size_t i = 0;
                i < node->repeat.body_count;
                i++
            ) {
                ast_free(
                    node->repeat.body[i]
                );
            }

            free(node->repeat.body);

            break;

        case AST_DURING:
            ast_free(node->during.condition);

            for (
                size_t i = 0;
                i < node->during.body_count;
                i++
            ) {
                ast_free(
                    node->during.body[i]
                );
            }

            free(node->during.body);

            break;

        case AST_NUMBER:
            break;

        case AST_STRING:
            free(node->string.value);

            break;

        case AST_BOOLEAN:
            break;

        case AST_EMPTY:
            break;

        case AST_VARIABLE:
            free(node->variable.name);

            break;

        case AST_BINARY:
            ast_free(node->binary.left);
            ast_free(node->binary.right);

            break;
    }

    free(node);
}
