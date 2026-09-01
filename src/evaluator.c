#include "evaluator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {
    AstNode *program;
    Environment *global;
} EvaluatorRuntime;

typedef enum {
    EXEC_OK,
    EXEC_RETURN,
    EXEC_ERROR
} ExecStatus;

static int is_truthy(Value value)
{
    switch (value.type) {
        case VALUE_EMPTY:
            return 0;

        case VALUE_NUMBER:
            return value.number != 0;

        case VALUE_STRING:
            return value.string &&
                   value.string[0] != '\0';

        case VALUE_BOOLEAN:
            return value.boolean != 0;
    }

    return 0;
}

static int values_equal(
    Value left,
    Value right
)
{
    if (
        left.type == VALUE_NUMBER &&
        right.type == VALUE_NUMBER
    ) {
        return left.number == right.number;
    }

    if (
        left.type == VALUE_BOOLEAN &&
        right.type == VALUE_BOOLEAN
    ) {
        return left.boolean == right.boolean;
    }

    if (
        left.type == VALUE_STRING &&
        right.type == VALUE_STRING
    ) {
        if (!left.string || !right.string)
            return left.string == right.string;

        return strcmp(
            left.string,
            right.string
        ) == 0;
    }

    if (
        left.type == VALUE_EMPTY &&
        right.type == VALUE_EMPTY
    ) {
        return 1;
    }

    return 0;
}

static int evaluate_node(
    AstNode *node,
    Environment *environment,
    EvaluatorRuntime *runtime,
    Value *result
);

static ExecStatus evaluate_statement(
    AstNode *node,
    Environment *environment,
    EvaluatorRuntime *runtime,
    int allow_return,
    Value *return_value
);

static int evaluate_call(
    AstNode *node,
    Environment *environment,
    EvaluatorRuntime *runtime,
    Value *result
);

static int evaluate_binary(
    AstNode *node,
    Environment *environment,
    EvaluatorRuntime *runtime,
    Value *result
)
{
    Value left;
    Value right;

    if (!evaluate_node(
        node->binary.left,
        environment,
        runtime,
        &left
    )) {
        return 0;
    }

    if (!evaluate_node(
        node->binary.right,
        environment,
        runtime,
        &right
    )) {
        value_free(&left);
        return 0;
    }

    switch (node->binary.operator) {
        case BINARY_ADD:
            if (
                left.type == VALUE_NUMBER &&
                right.type == VALUE_NUMBER
            ) {
                *result =
                    value_number(
                        left.number +
                        right.number
                    );
            } else if (
                left.type == VALUE_STRING &&
                right.type == VALUE_STRING
            ) {
                size_t left_length =
                    left.string
                        ? strlen(left.string)
                        : 0;

                size_t right_length =
                    right.string
                        ? strlen(right.string)
                        : 0;

                char *combined =
                    malloc(
                        left_length +
                        right_length +
                        1
                    );

                if (!combined) {
                    value_free(&left);
                    value_free(&right);
                    return 0;
                }

                if (left.string)
                    memcpy(
                        combined,
                        left.string,
                        left_length
                    );

                if (right.string)
                    memcpy(
                        combined +
                        left_length,
                        right.string,
                        right_length
                    );

                combined[
                    left_length +
                    right_length
                ] = '\0';

                *result =
                    value_string(combined);

                free(combined);
            } else {
                fprintf(
                    stderr,
                    "Milk error: cannot add these values\n"
                );

                value_free(&left);
                value_free(&right);

                return 0;
            }

            break;

        case BINARY_SUBTRACT:
            if (
                left.type != VALUE_NUMBER ||
                right.type != VALUE_NUMBER
            ) {
                fprintf(
                    stderr,
                    "Milk error: subtraction needs numbers\n"
                );

                value_free(&left);
                value_free(&right);

                return 0;
            }

            *result =
                value_number(
                    left.number -
                    right.number
                );

            break;

        case BINARY_MULTIPLY:
            if (
                left.type != VALUE_NUMBER ||
                right.type != VALUE_NUMBER
            ) {
                fprintf(
                    stderr,
                    "Milk error: multiplication needs numbers\n"
                );

                value_free(&left);
                value_free(&right);

                return 0;
            }

            *result =
                value_number(
                    left.number *
                    right.number
                );

            break;

        case BINARY_DIVIDE:
            if (
                left.type != VALUE_NUMBER ||
                right.type != VALUE_NUMBER
            ) {
                fprintf(
                    stderr,
                    "Milk error: division needs numbers\n"
                );

                value_free(&left);
                value_free(&right);

                return 0;
            }

            if (right.number == 0) {
                fprintf(
                    stderr,
                    "Milk error: cannot divide by zero\n"
                );

                value_free(&left);
                value_free(&right);

                return 0;
            }

            *result =
                value_number(
                    left.number /
                    right.number
                );

            break;

        case BINARY_MODULO:
            if (
                left.type != VALUE_NUMBER ||
                right.type != VALUE_NUMBER
            ) {
                fprintf(
                    stderr,
                    "Milk error: modulo needs numbers\n"
                );

                value_free(&left);
                value_free(&right);

                return 0;
            }

            if (right.number == 0) {
                fprintf(
                    stderr,
                    "Milk error: cannot modulo by zero\n"
                );

                value_free(&left);
                value_free(&right);

                return 0;
            }

            *result =
                value_number(
                    fmod(
                        left.number,
                        right.number
                    )
                );

            break;

        case BINARY_EQUAL:
            *result =
                value_boolean(
                    values_equal(
                        left,
                        right
                    )
                );

            break;

        case BINARY_NOT_EQUAL:
            *result =
                value_boolean(
                    !values_equal(
                        left,
                        right
                    )
                );

            break;

        case BINARY_LESS:
            if (
                left.type != VALUE_NUMBER ||
                right.type != VALUE_NUMBER
            ) {
                fprintf(
                    stderr,
                    "Milk error: comparison needs numbers\n"
                );

                value_free(&left);
                value_free(&right);

                return 0;
            }

            *result =
                value_boolean(
                    left.number <
                    right.number
                );

            break;

        case BINARY_LESS_EQUAL:
            if (
                left.type != VALUE_NUMBER ||
                right.type != VALUE_NUMBER
            ) {
                fprintf(
                    stderr,
                    "Milk error: comparison needs numbers\n"
                );

                value_free(&left);
                value_free(&right);

                return 0;
            }

            *result =
                value_boolean(
                    left.number <=
                    right.number
                );

            break;

        case BINARY_GREATER:
            if (
                left.type != VALUE_NUMBER ||
                right.type != VALUE_NUMBER
            ) {
                fprintf(
                    stderr,
                    "Milk error: comparison needs numbers\n"
                );

                value_free(&left);
                value_free(&right);

                return 0;
            }

            *result =
                value_boolean(
                    left.number >
                    right.number
                );

            break;

        case BINARY_GREATER_EQUAL:
            if (
                left.type != VALUE_NUMBER ||
                right.type != VALUE_NUMBER
            ) {
                fprintf(
                    stderr,
                    "Milk error: comparison needs numbers\n"
                );

                value_free(&left);
                value_free(&right);

                return 0;
            }

            *result =
                value_boolean(
                    left.number >=
                    right.number
                );

            break;
    }

    value_free(&left);
    value_free(&right);

    return 1;
}

static int evaluate_string(
    const char *input,
    Environment *environment,
    Value *result
)
{
    if (!input)
        return 0;

    size_t capacity =
        strlen(input) + 1;

    char *output =
        malloc(capacity);

    if (!output)
        return 0;

    size_t output_length = 0;

    for (
        size_t i = 0;
        input[i] != '\0';
    ) {
        if (
            input[i] == '{'
        ) {
            size_t start =
                i + 1;

            size_t end =
                start;

            while (
                input[end] != '\0' &&
                input[end] != '}'
            ) {
                end++;
            }

            if (input[end] == '}') {
                size_t name_length =
                    end - start;

                char *name =
                    malloc(
                        name_length + 1
                    );

                if (!name) {
                    free(output);
                    return 0;
                }

                memcpy(
                    name,
                    input + start,
                    name_length
                );

                name[name_length] =
                    '\0';

                Value value;

                if (
                    environment_get(
                        environment,
                        name,
                        &value
                    )
                ) {
                    char buffer[128];

                    if (
                        value.type ==
                        VALUE_STRING
                    ) {
                        size_t length =
                            value.string
                                ? strlen(value.string)
                                : 0;

                        while (
                            output_length +
                            length + 1 >
                            capacity
                        ) {
                            capacity *= 2;

                            char *grown =
                                realloc(
                                    output,
                                    capacity
                                );

                            if (!grown) {
                                value_free(&value);
                                free(name);
                                free(output);
                                return 0;
                            }

                            output = grown;
                        }

                        if (value.string)
                            memcpy(
                                output +
                                output_length,
                                value.string,
                                length
                            );

                        output_length +=
                            length;
                    } else if (
                        value.type ==
                        VALUE_NUMBER
                    ) {
                        snprintf(
                            buffer,
                            sizeof(buffer),
                            "%g",
                            value.number
                        );

                        size_t length =
                            strlen(buffer);

                        while (
                            output_length +
                            length + 1 >
                            capacity
                        ) {
                            capacity *= 2;

                            char *grown =
                                realloc(
                                    output,
                                    capacity
                                );

                            if (!grown) {
                                value_free(&value);
                                free(name);
                                free(output);
                                return 0;
                            }

                            output = grown;
                        }

                        memcpy(
                            output +
                            output_length,
                            buffer,
                            length
                        );

                        output_length +=
                            length;
                    } else if (
                        value.type ==
                        VALUE_BOOLEAN
                    ) {
                        const char *text =
                            value.boolean
                                ? "yes"
                                : "no";

                        size_t length =
                            strlen(text);

                        while (
                            output_length +
                            length + 1 >
                            capacity
                        ) {
                            capacity *= 2;

                            char *grown =
                                realloc(
                                    output,
                                    capacity
                                );

                            if (!grown) {
                                value_free(&value);
                                free(name);
                                free(output);
                                return 0;
                            }

                            output = grown;
                        }

                        memcpy(
                            output +
                            output_length,
                            text,
                            length
                        );

                        output_length +=
                            length;
                    }

                    value_free(&value);
                }

                free(name);

                i = end + 1;
                continue;
            }
        }

        while (
            output_length + 2 >
            capacity
        ) {
            capacity *= 2;

            char *grown =
                realloc(
                    output,
                    capacity
                );

            if (!grown) {
                free(output);
                return 0;
            }

            output = grown;
        }

        output[
            output_length++
        ] = input[i++];

        if (
            input[i - 1] == '\\' &&
            input[i] != '\0'
        ) {
            output[
                output_length - 1
            ] = input[i++];

        }
    }

    output[output_length] =
        '\0';

    *result =
        value_string(output);

    free(output);

    return 1;
}

static int evaluate_node(
    AstNode *node,
    Environment *environment,
    EvaluatorRuntime *runtime,
    Value *result
)
{
    if (!node)
        return 0;

    switch (node->type) {
        case AST_NUMBER:
            *result =
                value_number(
                    node->number.value
                );

            return 1;

        case AST_STRING:
            return evaluate_string(
                node->string.value,
                environment,
                result
            );

        case AST_BOOLEAN:
            *result =
                value_boolean(
                    node->boolean.value
                );

            return 1;

        case AST_EMPTY:
            *result =
                value_empty();

            return 1;

        case AST_VARIABLE:
            if (
                !environment_get(
                    environment,
                    node->variable.name,
                    result
                )
            ) {
                fprintf(
                    stderr,
                    "Milk error: unknown value '%s'\n",
                    node->variable.name
                );

                return 0;
            }

            return 1;

        case AST_BINARY:
            return evaluate_binary(
                node,
                environment,
                runtime,
                result
            );

        case AST_CALL:
            return evaluate_call(
                node,
                environment,
                runtime,
                result
            );

        default:
            fprintf(
                stderr,
                "Milk error: expression expected\n"
            );

            return 0;
    }
}

static AstNode *find_function(
    AstNode *program,
    const char *name
)
{
    if (!program || program->type != AST_PROGRAM)
        return NULL;

    for (size_t i = 0; i < program->program.count; i++) {
        AstNode *statement = program->program.statements[i];

        if (statement->type == AST_FUNCTION &&
            strcmp(statement->function.name, name) == 0)
            return statement;
    }

    return NULL;
}

static int evaluate_call(
    AstNode *node,
    Environment *environment,
    EvaluatorRuntime *runtime,
    Value *result
)
{
    AstNode *function =
        find_function(runtime->program, node->call.name);

    if (!function) {
        fprintf(
            stderr,
            "Milk error: unknown function '%s'\n",
            node->call.name
        );
        return 0;
    }

    if (node->call.argument_count !=
        function->function.parameter_count) {
        fprintf(
            stderr,
            "Milk error: function '%s' expects %zu argument%s, got %zu\n",
            node->call.name,
            function->function.parameter_count,
            function->function.parameter_count == 1 ? "" : "s",
            node->call.argument_count
        );
        return 0;
    }

    size_t count = node->call.argument_count;
    Value *arguments = NULL;

    if (count > 0) {
        arguments = calloc(count, sizeof(Value));

        if (!arguments)
            return 0;
    }

    for (size_t i = 0; i < count; i++) {
        if (!evaluate_node(
            node->call.arguments[i],
            environment,
            runtime,
            &arguments[i]
        )) {
            for (size_t j = 0; j < i; j++)
                value_free(&arguments[j]);
            free(arguments);
            return 0;
        }
    }

    Environment local;
    environment_init_child(&local, runtime->global);

    for (size_t i = 0; i < count; i++) {
        if (!environment_set(
            &local,
            function->function.parameters[i],
            arguments[i]
        )) {
            arguments[i] = value_empty();
            for (size_t j = i + 1; j < count; j++)
                value_free(&arguments[j]);
            free(arguments);
            environment_free(&local);
            return 0;
        }
        arguments[i] = value_empty();
    }

    free(arguments);

    Value returned = value_empty();
    ExecStatus status = evaluate_statement(
        function->function.body,
        &local,
        runtime,
        1,
        &returned
    );

    environment_free(&local);

    if (status == EXEC_ERROR) {
        value_free(&returned);
        return 0;
    }

    *result = returned;
    return 1;
}

static ExecStatus evaluate_statement(
    AstNode *node,
    Environment *environment,
    EvaluatorRuntime *runtime,
    int allow_return,
    Value *return_value
)
{
    if (!node)
        return EXEC_ERROR;

    switch (node->type) {
        case AST_FUNCTION:
            return EXEC_OK;

        case AST_GIVE:
        {
            if (!allow_return) {
                fprintf(
                    stderr,
                    "Milk error: 'give' can only be used inside a function\n"
                );
                return EXEC_ERROR;
            }

            if (!evaluate_node(
                node->give.value,
                environment,
                runtime,
                return_value
            )) {
                return EXEC_ERROR;
            }

            return EXEC_RETURN;
        }

        case AST_CALL:
        {
            Value value;

            if (!evaluate_node(
                node,
                environment,
                runtime,
                &value
            )) {
                return EXEC_ERROR;
            }

            value_free(&value);
            return EXEC_OK;
        }

        case AST_ASSIGNMENT:
        {
            Value value;

            if (!evaluate_node(
                node->assignment.value,
                environment,
                runtime,
                &value
            )) {
                return EXEC_ERROR;
            }

            if (
                !environment_set(
                    environment,
                    node->assignment.name,
                    value
                )
            ) {
                value_free(&value);
                return EXEC_ERROR;
            }

            return EXEC_OK;
        }

        case AST_SHOW:
        {
            Value value;

            if (!evaluate_node(
                node->show.value,
                environment,
                runtime,
                &value
            )) {
                return EXEC_ERROR;
            }

            switch (value.type) {
                case VALUE_EMPTY:
                    printf("empty\n");
                    break;

                case VALUE_NUMBER:
                    printf(
                        "%g\n",
                        value.number
                    );
                    break;

                case VALUE_STRING:
                    printf(
                        "%s\n",
                        value.string
                            ? value.string
                            : ""
                    );
                    break;

                case VALUE_BOOLEAN:
                    printf(
                        "%s\n",
                        value.boolean
                            ? "yes"
                            : "no"
                    );
                    break;
            }

            value_free(&value);

            return EXEC_OK;
        }

        case AST_CHECK:
        {
            Value condition;

            if (!evaluate_node(
                node->check.condition,
                environment,
                runtime,
                &condition
            )) {
                return EXEC_ERROR;
            }

            int truth =
                is_truthy(condition);

            value_free(&condition);

            if (truth) {
                for (
                    size_t i = 0;
                    i < node->check.body_count;
                    i++
                ) {
                    ExecStatus status = evaluate_statement(
                        node->check.body[i],
                        environment,
                        runtime,
                        allow_return,
                        return_value
                    );

                    if (status != EXEC_OK)
                        return status;
                }
            } else if (
                node->check.otherwise
            ) {
                ExecStatus status = evaluate_statement(
                    node->check.otherwise,
                    environment,
                    runtime,
                    allow_return,
                    return_value
                );

                if (status != EXEC_OK)
                    return status;
            }

            return EXEC_OK;
        }

        case AST_REPEAT:
        {
            Value times;

            if (!evaluate_node(
                node->repeat.times,
                environment,
                runtime,
                &times
            )) {
                return EXEC_ERROR;
            }

            if (
                times.type != VALUE_NUMBER
            ) {
                fprintf(
                    stderr,
                    "Milk error: repeat needs a number\n"
                );

                value_free(&times);

                return EXEC_ERROR;
            }

            int count =
                (int)times.number;

            value_free(&times);

            if (count < 0)
                count = 0;

            for (
                int i = 0;
                i < count;
                i++
            ) {
                for (
                    size_t j = 0;
                    j < node->repeat.body_count;
                    j++
                ) {
                    ExecStatus status = evaluate_statement(
                        node->repeat.body[j],
                        environment,
                        runtime,
                        allow_return,
                        return_value
                    );

                    if (status != EXEC_OK)
                        return status;
                }
            }

            return EXEC_OK;
        }

        case AST_DURING:
        {
            for (;;) {
                Value condition;

                if (!evaluate_node(
                    node->during.condition,
                    environment,
                    runtime,
                    &condition
                )) {
                    return EXEC_ERROR;
                }

                int truth =
                    is_truthy(condition);

                value_free(&condition);

                if (!truth)
                    break;

                for (
                    size_t i = 0;
                    i < node->during.body_count;
                    i++
                ) {
                    ExecStatus status = evaluate_statement(
                        node->during.body[i],
                        environment,
                        runtime,
                        allow_return,
                        return_value
                    );

                    if (status != EXEC_OK)
                        return status;
                }
            }

            return EXEC_OK;
        }

        case AST_PROGRAM:
            for (
                size_t i = 0;
                i < node->program.count;
                i++
            ) {
                ExecStatus status = evaluate_statement(
                    node->program.statements[i],
                    environment,
                    runtime,
                    allow_return,
                    return_value
                );

                if (status != EXEC_OK)
                    return status;
            }

            return EXEC_OK;

        default:
            return EXEC_ERROR;
    }
}

int evaluator_run(
    AstNode *program,
    Environment *environment
)
{
    if (!program || !environment)
        return 0;

    EvaluatorRuntime runtime;
    runtime.program = program;
    runtime.global = environment;

    ExecStatus status = evaluate_statement(
        program,
        environment,
        &runtime,
        0,
        NULL
    );

    if (status == EXEC_RETURN) {
        fprintf(
            stderr,
            "Milk error: 'give' can only be used inside a function\n"
        );
        return 0;
    }

    return status == EXEC_OK;
}
