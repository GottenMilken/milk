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

static int add_node(
    AstNode ***items,
    size_t *count,
    size_t *capacity,
    AstNode *item
)
{
    if (*count >= *capacity) {
        size_t next_capacity =
            *capacity == 0 ? 4 : *capacity * 2;

        AstNode **next = realloc(
            *items,
            next_capacity * sizeof(AstNode *)
        );

        if (!next)
            return 0;

        *items = next;
        *capacity = next_capacity;
    }

    (*items)[(*count)++] = item;
    return 1;
}

static int add_string(
    char ***items,
    size_t *count,
    size_t *capacity,
    const char *item
)
{
    if (*count >= *capacity) {
        size_t next_capacity =
            *capacity == 0 ? 4 : *capacity * 2;

        char **next = realloc(
            *items,
            next_capacity * sizeof(char *)
        );

        if (!next)
            return 0;

        *items = next;
        *capacity = next_capacity;
    }

    (*items)[*count] = copy_string(item);

    if (!(*items)[*count])
        return 0;

    (*count)++;
    return 1;
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
    AstNode *node = create_node(AST_ASSIGNMENT);

    if (!node)
        return NULL;

    node->assignment.name = copy_string(name);
    node->assignment.value = value;

    if (!node->assignment.name) {
        ast_free(node);
        return NULL;
    }

    return node;
}

AstNode *ast_show(AstNode *value)
{
    AstNode *node = create_node(AST_SHOW);

    if (!node)
        return NULL;

    node->show.value = value;
    return node;
}

AstNode *ast_check(AstNode *condition)
{
    AstNode *node = create_node(AST_CHECK);

    if (!node)
        return NULL;

    node->check.condition = condition;
    return node;
}

AstNode *ast_repeat(AstNode *times)
{
    AstNode *node = create_node(AST_REPEAT);

    if (!node)
        return NULL;

    node->repeat.times = times;
    return node;
}

AstNode *ast_during(AstNode *condition)
{
    AstNode *node = create_node(AST_DURING);

    if (!node)
        return NULL;

    node->during.condition = condition;
    return node;
}

AstNode *ast_function(const char *name)
{
    AstNode *node = create_node(AST_FUNCTION);

    if (!node)
        return NULL;

    node->function.name = copy_string(name);

    if (!node->function.name) {
        ast_free(node);
        return NULL;
    }

    return node;
}

AstNode *ast_give(AstNode *value)
{
    AstNode *node = create_node(AST_GIVE);

    if (!node)
        return NULL;

    node->give.value = value;
    return node;
}

AstNode *ast_call(const char *name)
{
    AstNode *node = create_node(AST_CALL);

    if (!node)
        return NULL;

    node->call.name = copy_string(name);

    if (!node->call.name) {
        ast_free(node);
        return NULL;
    }

    return node;
}

AstNode *ast_number(double value)
{
    AstNode *node = create_node(AST_NUMBER);

    if (!node)
        return NULL;

    node->number.value = value;
    return node;
}

AstNode *ast_string(const char *value)
{
    AstNode *node = create_node(AST_STRING);

    if (!node)
        return NULL;

    node->string.value = copy_string(value);

    if (!node->string.value) {
        ast_free(node);
        return NULL;
    }

    return node;
}

AstNode *ast_boolean(int value)
{
    AstNode *node = create_node(AST_BOOLEAN);

    if (!node)
        return NULL;

    node->boolean.value = value ? 1 : 0;
    return node;
}

AstNode *ast_empty(void)
{
    return create_node(AST_EMPTY);
}

AstNode *ast_variable(const char *name)
{
    AstNode *node = create_node(AST_VARIABLE);

    if (!node)
        return NULL;

    node->variable.name = copy_string(name);

    if (!node->variable.name) {
        ast_free(node);
        return NULL;
    }

    return node;
}

AstNode *ast_binary(
    BinaryOperator operator,
    AstNode *left,
    AstNode *right
)
{
    AstNode *node = create_node(AST_BINARY);

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

    if (!add_node(
        &program->program.statements,
        &program->program.count,
        &program->program.capacity,
        statement
    )) {
        ast_free(statement);
    }
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

    if (!add_node(
        &check->check.body,
        &check->check.body_count,
        &check->check.body_capacity,
        statement
    )) {
        ast_free(statement);
    }
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

    if (!add_node(
        &repeat->repeat.body,
        &repeat->repeat.body_count,
        &repeat->repeat.body_capacity,
        statement
    )) {
        ast_free(statement);
    }
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

    if (!add_node(
        &during->during.body,
        &during->during.body_count,
        &during->during.body_capacity,
        statement
    )) {
        ast_free(statement);
    }
}

void ast_function_add_parameter(
    AstNode *function,
    const char *parameter
)
{
    if (!function ||
        function->type != AST_FUNCTION ||
        !parameter)
        return;

    if (!add_string(
        &function->function.parameters,
        &function->function.parameter_count,
        &function->function.parameter_capacity,
        parameter
    )) {
        return;
    }
}

void ast_function_set_body(
    AstNode *function,
    AstNode *body
)
{
    if (!function ||
        function->type != AST_FUNCTION)
        return;

    function->function.body = body;
}

void ast_call_add_argument(
    AstNode *call,
    AstNode *argument
)
{
    if (!call ||
        call->type != AST_CALL ||
        !argument)
        return;

    if (!add_node(
        &call->call.arguments,
        &call->call.argument_count,
        &call->call.argument_capacity,
        argument
    )) {
        ast_free(argument);
    }
}

void ast_print(
    const AstNode *node,
    int indent
)
{
    if (!node)
        return;

    for (int i = 0; i < indent; i++)
        printf("  ");

    switch (node->type) {
        case AST_PROGRAM:
            printf("Program\n");
            for (size_t i = 0; i < node->program.count; i++)
                ast_print(node->program.statements[i], indent + 1);
            break;

        case AST_ASSIGNMENT:
            printf("Assignment: %s\n", node->assignment.name);
            ast_print(node->assignment.value, indent + 1);
            break;

        case AST_SHOW:
            printf("Show\n");
            ast_print(node->show.value, indent + 1);
            break;

        case AST_CHECK:
            printf("Check\n");
            for (int i = 0; i < indent + 1; i++)
                printf("  ");
            printf("Condition\n");
            ast_print(node->check.condition, indent + 2);
            for (int i = 0; i < indent + 1; i++)
                printf("  ");
            printf("Body\n");
            for (size_t i = 0; i < node->check.body_count; i++)
                ast_print(node->check.body[i], indent + 2);
            if (node->check.otherwise) {
                for (int i = 0; i < indent + 1; i++)
                    printf("  ");
                printf("Otherwise\n");
                ast_print(node->check.otherwise, indent + 2);
            }
            break;

        case AST_REPEAT:
            printf("Repeat\n");
            for (int i = 0; i < indent + 1; i++)
                printf("  ");
            printf("Times\n");
            ast_print(node->repeat.times, indent + 2);
            for (int i = 0; i < indent + 1; i++)
                printf("  ");
            printf("Body\n");
            for (size_t i = 0; i < node->repeat.body_count; i++)
                ast_print(node->repeat.body[i], indent + 2);
            break;

        case AST_DURING:
            printf("During\n");
            for (int i = 0; i < indent + 1; i++)
                printf("  ");
            printf("Condition\n");
            ast_print(node->during.condition, indent + 2);
            for (int i = 0; i < indent + 1; i++)
                printf("  ");
            printf("Body\n");
            for (size_t i = 0; i < node->during.body_count; i++)
                ast_print(node->during.body[i], indent + 2);
            break;

        case AST_FUNCTION:
            printf("Function: %s\n", node->function.name);
            for (int i = 0; i < indent + 1; i++)
                printf("  ");
            printf("Parameters");
            for (size_t i = 0; i < node->function.parameter_count; i++)
                printf(" %s", node->function.parameters[i]);
            printf("\n");
            ast_print(node->function.body, indent + 1);
            break;

        case AST_GIVE:
            printf("Give\n");
            ast_print(node->give.value, indent + 1);
            break;

        case AST_CALL:
            printf("Call: %s\n", node->call.name);
            for (size_t i = 0; i < node->call.argument_count; i++)
                ast_print(node->call.arguments[i], indent + 1);
            break;

        case AST_NUMBER:
            printf("Number: %g\n", node->number.value);
            break;

        case AST_STRING:
            printf("String: \"%s\"\n", node->string.value);
            break;

        case AST_BOOLEAN:
            printf(
                "Boolean: %s\n",
                node->boolean.value ? "yes" : "no"
            );
            break;

        case AST_EMPTY:
            printf("Empty\n");
            break;

        case AST_VARIABLE:
            printf("Variable: %s\n", node->variable.name);
            break;

        case AST_BINARY:
            printf("Binary\n");
            ast_print(node->binary.left, indent + 1);
            ast_print(node->binary.right, indent + 1);
            break;
    }
}

void ast_free(AstNode *node)
{
    if (!node)
        return;

    switch (node->type) {
        case AST_PROGRAM:
            for (size_t i = 0; i < node->program.count; i++)
                ast_free(node->program.statements[i]);
            free(node->program.statements);
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
            for (size_t i = 0; i < node->check.body_count; i++)
                ast_free(node->check.body[i]);
            free(node->check.body);
            ast_free(node->check.otherwise);
            break;

        case AST_REPEAT:
            ast_free(node->repeat.times);
            for (size_t i = 0; i < node->repeat.body_count; i++)
                ast_free(node->repeat.body[i]);
            free(node->repeat.body);
            break;

        case AST_DURING:
            ast_free(node->during.condition);
            for (size_t i = 0; i < node->during.body_count; i++)
                ast_free(node->during.body[i]);
            free(node->during.body);
            break;

        case AST_FUNCTION:
            free(node->function.name);
            for (size_t i = 0; i < node->function.parameter_count; i++)
                free(node->function.parameters[i]);
            free(node->function.parameters);
            ast_free(node->function.body);
            break;

        case AST_GIVE:
            ast_free(node->give.value);
            break;

        case AST_CALL:
            free(node->call.name);
            for (size_t i = 0; i < node->call.argument_count; i++)
                ast_free(node->call.arguments[i]);
            free(node->call.arguments);
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
