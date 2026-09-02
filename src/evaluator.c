#define _POSIX_C_SOURCE 200809L
#include "evaluator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/wait.h>

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
            return value.string && value.string[0] != '\0';
        case VALUE_LIST:
            return value.list.count != 0;
        case VALUE_BOOLEAN:
            return value.boolean != 0;
    }

    return 0;
}

static int values_equal(Value left, Value right)
{
    if (left.type == VALUE_LIST && right.type == VALUE_LIST) {
        if (left.list.count != right.list.count)
            return 0;

        for (size_t i = 0; i < left.list.count; i++) {
            if (!values_equal(left.list.items[i], right.list.items[i]))
                return 0;
        }

        return 1;
    }

    if (left.type == VALUE_NUMBER && right.type == VALUE_NUMBER)
        return left.number == right.number;

    if (left.type == VALUE_BOOLEAN && right.type == VALUE_BOOLEAN)
        return left.boolean == right.boolean;

    if (left.type == VALUE_STRING && right.type == VALUE_STRING) {
        if (!left.string || !right.string)
            return left.string == right.string;

        return strcmp(left.string, right.string) == 0;
    }

    if (left.type == VALUE_EMPTY && right.type == VALUE_EMPTY)
        return 1;

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

static int append_text(
    char **output,
    size_t *length,
    size_t *capacity,
    const char *text
)
{
    size_t text_length = text ? strlen(text) : 0;

    while (*length + text_length + 1 > *capacity) {
        *capacity *= 2;

        char *grown = realloc(*output, *capacity);
        if (!grown)
            return 0;

        *output = grown;
    }

    if (text_length > 0)
        memcpy(*output + *length, text, text_length);

    *length += text_length;
    (*output)[*length] = '\0';
    return 1;
}

static int append_value_text(
    char **output,
    size_t *length,
    size_t *capacity,
    Value value
)
{
    char buffer[128];

    switch (value.type) {
        case VALUE_EMPTY:
            return append_text(output, length, capacity, "empty");
        case VALUE_NUMBER:
            snprintf(buffer, sizeof(buffer), "%g", value.number);
            return append_text(output, length, capacity, buffer);
        case VALUE_STRING:
            return append_text(output, length, capacity, value.string);
        case VALUE_BOOLEAN:
            return append_text(
                output,
                length,
                capacity,
                value.boolean ? "yes" : "no"
            );
        case VALUE_LIST:
            if (!append_text(output, length, capacity, "["))
                return 0;

            for (size_t i = 0; i < value.list.count; i++) {
                if (i > 0 &&
                    !append_text(output, length, capacity, ", "))
                    return 0;

                if (!append_value_text(
                    output,
                    length,
                    capacity,
                    value.list.items[i]
                ))
                    return 0;
            }

            return append_text(output, length, capacity, "]");
    }

    return 0;
}

static int evaluate_string(
    const char *input,
    Environment *environment,
    Value *result
)
{
    if (!input)
        return 0;

    size_t capacity = strlen(input) + 1;
    char *output = malloc(capacity);

    if (!output)
        return 0;

    output[0] = '\0';
    size_t output_length = 0;

    for (size_t i = 0; input[i] != '\0';) {
        if (input[i] == '{') {
            size_t start = i + 1;
            size_t end = start;

            while (
                input[end] != '\0' &&
                input[end] != '}'
            ) {
                end++;
            }

            if (input[end] == '}') {
                size_t name_length = end - start;
                char *name = malloc(name_length + 1);

                if (!name) {
                    free(output);
                    return 0;
                }

                memcpy(name, input + start, name_length);
                name[name_length] = '\0';

                Value value;
                if (environment_get(environment, name, &value)) {
                    if (!append_value_text(
                        &output,
                        &output_length,
                        &capacity,
                        value
                    )) {
                        value_free(&value);
                        free(name);
                        free(output);
                        return 0;
                    }

                    value_free(&value);
                } else if (!append_text(
                    &output,
                    &output_length,
                    &capacity,
                    input + i
                )) {
                    free(name);
                    free(output);
                    return 0;
                }

                free(name);
                i = end + 1;
                continue;
            }
        }

        char character[2];
        character[0] = input[i++];
        character[1] = '\0';

        if (!append_text(
            &output,
            &output_length,
            &capacity,
            character
        )) {
            free(output);
            return 0;
        }
    }

    *result = value_string(output);
    free(output);

    return result->string != NULL || input[0] == '\0';
}

static int evaluate_scalar_binary(
    BinaryOperator operator,
    Value left,
    Value right,
    Value *result
)
{
    switch (operator) {
        case BINARY_ADD:
            if (
                left.type == VALUE_NUMBER &&
                right.type == VALUE_NUMBER
            ) {
                *result = value_number(left.number + right.number);
                return 1;
            }

            if (
                left.type == VALUE_STRING &&
                right.type == VALUE_STRING
            ) {
                size_t left_length = left.string ? strlen(left.string) : 0;
                size_t right_length = right.string ? strlen(right.string) : 0;
                char *combined = malloc(left_length + right_length + 1);

                if (!combined)
                    return 0;

                if (left.string)
                    memcpy(combined, left.string, left_length);
                if (right.string)
                    memcpy(combined + left_length, right.string, right_length);

                combined[left_length + right_length] = '\0';
                *result = value_string(combined);
                free(combined);
                return result->string != NULL ||
                       (left_length + right_length == 0);
            }

            fprintf(stderr, "Milk error: cannot add these values\n");
            return 0;

        case BINARY_SUBTRACT:
            if (left.type != VALUE_NUMBER || right.type != VALUE_NUMBER) {
                fprintf(stderr, "Milk error: subtraction needs numbers\n");
                return 0;
            }

            *result = value_number(left.number - right.number);
            return 1;

        case BINARY_MULTIPLY:
            if (left.type != VALUE_NUMBER || right.type != VALUE_NUMBER) {
                fprintf(stderr, "Milk error: multiplication needs numbers\n");
                return 0;
            }

            *result = value_number(left.number * right.number);
            return 1;

        case BINARY_DIVIDE:
            if (left.type != VALUE_NUMBER || right.type != VALUE_NUMBER) {
                fprintf(stderr, "Milk error: division needs numbers\n");
                return 0;
            }

            if (right.number == 0) {
                fprintf(stderr, "Milk error: cannot divide by zero\n");
                return 0;
            }

            *result = value_number(left.number / right.number);
            return 1;

        case BINARY_MODULO:
            if (left.type != VALUE_NUMBER || right.type != VALUE_NUMBER) {
                fprintf(stderr, "Milk error: modulo needs numbers\n");
                return 0;
            }

            if (right.number == 0) {
                fprintf(stderr, "Milk error: cannot modulo by zero\n");
                return 0;
            }

            *result = value_number(fmod(left.number, right.number));
            return 1;

        case BINARY_EQUAL:
            *result = value_boolean(values_equal(left, right));
            return 1;

        case BINARY_NOT_EQUAL:
            *result = value_boolean(!values_equal(left, right));
            return 1;

        case BINARY_LESS:
            if (left.type != VALUE_NUMBER || right.type != VALUE_NUMBER) {
                fprintf(stderr, "Milk error: comparison needs numbers\n");
                return 0;
            }

            *result = value_boolean(left.number < right.number);
            return 1;

        case BINARY_LESS_EQUAL:
            if (left.type != VALUE_NUMBER || right.type != VALUE_NUMBER) {
                fprintf(stderr, "Milk error: comparison needs numbers\n");
                return 0;
            }

            *result = value_boolean(left.number <= right.number);
            return 1;

        case BINARY_GREATER:
            if (left.type != VALUE_NUMBER || right.type != VALUE_NUMBER) {
                fprintf(stderr, "Milk error: comparison needs numbers\n");
                return 0;
            }

            *result = value_boolean(left.number > right.number);
            return 1;

        case BINARY_GREATER_EQUAL:
            if (left.type != VALUE_NUMBER || right.type != VALUE_NUMBER) {
                fprintf(stderr, "Milk error: comparison needs numbers\n");
                return 0;
            }

            *result = value_boolean(left.number >= right.number);
            return 1;
    }

    return 0;
}

static int evaluate_binary_values(
    BinaryOperator operator,
    Value left,
    Value right,
    Value *result
)
{
    if (
        left.type != VALUE_LIST &&
        right.type != VALUE_LIST
    ) {
        return evaluate_scalar_binary(operator, left, right, result);
    }

    if (left.type == VALUE_LIST && right.type == VALUE_LIST) {
        if (left.list.count != right.list.count) {
            fprintf(stderr, "Milk error: data flow needs equal list sizes\n");
            return 0;
        }

        Value *items = NULL;

        if (left.list.count > 0) {
            items = calloc(left.list.count, sizeof(Value));
            if (!items)
                return 0;
        }

        for (size_t i = 0; i < left.list.count; i++) {
            if (!evaluate_binary_values(
                operator,
                left.list.items[i],
                right.list.items[i],
                &items[i]
            )) {
                for (size_t j = 0; j < i; j++)
                    value_free(&items[j]);
                free(items);
                return 0;
            }
        }

        *result = value_list(items, left.list.count);

        for (size_t i = 0; i < left.list.count; i++)
            value_free(&items[i]);
        free(items);

        if (
            left.list.count > 0 &&
            result->type != VALUE_LIST
        )
            return 0;

        return 1;
    }

    if (left.type == VALUE_LIST) {
        Value *items = NULL;

        if (left.list.count > 0) {
            items = calloc(left.list.count, sizeof(Value));
            if (!items)
                return 0;
        }

        for (size_t i = 0; i < left.list.count; i++) {
            if (!evaluate_binary_values(
                operator,
                left.list.items[i],
                right,
                &items[i]
            )) {
                for (size_t j = 0; j < i; j++)
                    value_free(&items[j]);
                free(items);
                return 0;
            }
        }

        *result = value_list(items, left.list.count);

        for (size_t i = 0; i < left.list.count; i++)
            value_free(&items[i]);
        free(items);
        return result->type == VALUE_LIST || left.list.count == 0;
    }

    Value *items = NULL;

    if (right.list.count > 0) {
        items = calloc(right.list.count, sizeof(Value));
        if (!items)
            return 0;
    }

    for (size_t i = 0; i < right.list.count; i++) {
        if (!evaluate_binary_values(
            operator,
            left,
            right.list.items[i],
            &items[i]
        )) {
            for (size_t j = 0; j < i; j++)
                value_free(&items[j]);
            free(items);
            return 0;
        }
    }

    *result = value_list(items, right.list.count);

    for (size_t i = 0; i < right.list.count; i++)
        value_free(&items[i]);
    free(items);
    return result->type == VALUE_LIST || right.list.count == 0;
}

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
    ))
        return 0;

    if (!evaluate_node(
        node->binary.right,
        environment,
        runtime,
        &right
    )) {
        value_free(&left);
        return 0;
    }

    int success = evaluate_binary_values(
        node->binary.operator,
        left,
        right,
        result
    );

    value_free(&left);
    value_free(&right);
    return success;
}

static int evaluate_run(
    AstNode *node,
    Environment *environment,
    EvaluatorRuntime *runtime,
    Value *result
)
{
    Value command;

    if (!evaluate_node(node->run.command, environment, runtime, &command))
        return 0;

    if (command.type != VALUE_STRING) {
        fprintf(stderr, "Milk error: run needs a string command\n");
        value_free(&command);
        return 0;
    }

    FILE *pipe = popen(command.string ? command.string : "", "r");
    value_free(&command);

    if (!pipe) {
        fprintf(stderr, "Milk error: could not run shell command\n");
        return 0;
    }

    size_t capacity = 256;
    size_t length = 0;
    char *output = malloc(capacity);
    if (!output) {
        pclose(pipe);
        return 0;
    }
    output[0] = '\0';

    char buffer[4096];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), pipe)) > 0) {
        if (length + bytes + 1 > capacity) {
            size_t next_capacity = capacity;
            while (length + bytes + 1 > next_capacity)
                next_capacity *= 2;

            char *grown = realloc(output, next_capacity);
            if (!grown) {
                free(output);
                pclose(pipe);
                return 0;
            }
            output = grown;
            capacity = next_capacity;
        }

        memcpy(output + length, buffer, bytes);
        length += bytes;
        output[length] = '\0';
    }

    int read_error = ferror(pipe);
    int status = pclose(pipe);

    if (read_error) {
        free(output);
        fprintf(stderr, "Milk error: could not read shell command output\n");
        return 0;
    }

    if (status == -1) {
        free(output);
        fprintf(stderr, "Milk error: could not run shell command\n");
        return 0;
    }

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        while (length > 0 &&
               (output[length - 1] == '\n' ||
                output[length - 1] == '\r')) {
            output[--length] = '\0';
        }

        *result = value_string(output);
        free(output);
        return result->type == VALUE_STRING &&
               (result->string != NULL || length == 0);
    }

    if (WIFEXITED(status)) {
        fprintf(stderr, "Milk error: shell command exited with status %d\n",
                WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        fprintf(stderr, "Milk error: shell command terminated by signal %d\n",
                WTERMSIG(status));
    } else {
        fprintf(stderr, "Milk error: shell command failed\n");
    }

    free(output);
    return 0;
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
            *result = value_number(node->number.value);
            return 1;

        case AST_STRING:
            return evaluate_string(
                node->string.value,
                environment,
                result
            );

        case AST_LIST:
        {
            Value *items = NULL;

            if (node->list.count > 0) {
                items = calloc(node->list.count, sizeof(Value));
                if (!items)
                    return 0;
            }

            for (size_t i = 0; i < node->list.count; i++) {
                if (!evaluate_node(
                    node->list.items[i],
                    environment,
                    runtime,
                    &items[i]
                )) {
                    for (size_t j = 0; j < i; j++)
                        value_free(&items[j]);
                    free(items);
                    return 0;
                }
            }

            *result = value_list(items, node->list.count);

            for (size_t i = 0; i < node->list.count; i++)
                value_free(&items[i]);
            free(items);

            return result->type == VALUE_LIST || node->list.count == 0;
        }

        case AST_BOOLEAN:
            *result = value_boolean(node->boolean.value);
            return 1;

        case AST_EMPTY:
            *result = value_empty();
            return 1;

        case AST_VARIABLE:
            if (!environment_get(
                environment,
                node->variable.name,
                result
            )) {
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

        case AST_RUN:
            return evaluate_run(
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
            fprintf(stderr, "Milk error: expression expected\n");
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

        if (
            statement->type == AST_FUNCTION &&
            strcmp(statement->function.name, name) == 0
        )
            return statement;
    }

    return NULL;
}

static int evaluate_scalar_call(
    AstNode *node,
    EvaluatorRuntime *runtime,
    AstNode *function,
    Value *arguments,
    Value *result
)
{
    Environment local;
    environment_init_child(&local, runtime->global);

    for (size_t i = 0; i < node->call.argument_count; i++) {
        if (!environment_set(
            &local,
            function->function.parameters[i],
            arguments[i]
        )) {
            environment_free(&local);
            return 0;
        }

        arguments[i] = value_empty();
    }

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

static int evaluate_call(
    AstNode *node,
    Environment *environment,
    EvaluatorRuntime *runtime,
    Value *result
)
{
    AstNode *function = find_function(
        runtime->program,
        node->call.name
    );

    if (!function) {
        fprintf(
            stderr,
            "Milk error: unknown function '%s'\n",
            node->call.name
        );
        return 0;
    }

    if (
        node->call.argument_count !=
        function->function.parameter_count
    ) {
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

    size_t flow_count = 0;
    int has_flow = 0;

    for (size_t i = 0; i < count; i++) {
        if (arguments[i].type == VALUE_LIST) {
            if (!has_flow) {
                flow_count = arguments[i].list.count;
                has_flow = 1;
            } else if (flow_count != arguments[i].list.count) {
                fprintf(
                    stderr,
                    "Milk error: data flow needs equal list sizes\n"
                );
                for (size_t j = 0; j < count; j++)
                    value_free(&arguments[j]);
                free(arguments);
                return 0;
            }
        }
    }

    if (!has_flow) {
        int success = evaluate_scalar_call(
            node,
            runtime,
            function,
            arguments,
            result
        );
        for (size_t i = 0; i < count; i++)
            value_free(&arguments[i]);
        free(arguments);
        return success;
    }

    Value *outputs = NULL;

    if (flow_count > 0) {
        outputs = calloc(flow_count, sizeof(Value));
        if (!outputs) {
            for (size_t i = 0; i < count; i++)
                value_free(&arguments[i]);
            free(arguments);
            return 0;
        }
    }

    for (size_t index = 0; index < flow_count; index++) {
        Value *flow_arguments = NULL;

        if (count > 0) {
            flow_arguments = calloc(count, sizeof(Value));
            if (!flow_arguments)
                goto flow_error;
        }

        for (size_t i = 0; i < count; i++) {
            if (arguments[i].type == VALUE_LIST)
                flow_arguments[i] = value_copy(
                    &arguments[i].list.items[index]
                );
            else
                flow_arguments[i] = value_copy(&arguments[i]);
        }

        if (!evaluate_scalar_call(
            node,
            runtime,
            function,
            flow_arguments,
            &outputs[index]
        )) {
            for (size_t i = 0; i < count; i++)
                value_free(&flow_arguments[i]);
            free(flow_arguments);
            goto flow_error;
        }

        free(flow_arguments);
    }

    *result = value_list(outputs, flow_count);

    for (size_t i = 0; i < flow_count; i++)
        value_free(&outputs[i]);
    free(outputs);

    for (size_t i = 0; i < count; i++)
        value_free(&arguments[i]);
    free(arguments);

    return result->type == VALUE_LIST;

flow_error:
    if (outputs) {
        for (size_t i = 0; i < flow_count; i++)
            value_free(&outputs[i]);
        free(outputs);
    }

    for (size_t i = 0; i < count; i++)
        value_free(&arguments[i]);
    free(arguments);
    return 0;
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
            ))
                return EXEC_ERROR;

            return EXEC_RETURN;

        case AST_CALL:
        {
            Value value;

            if (!evaluate_node(
                node,
                environment,
                runtime,
                &value
            ))
                return EXEC_ERROR;

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
            ))
                return EXEC_ERROR;

            if (!environment_set(
                environment,
                node->assignment.name,
                value
            )) {
                value_free(&value);
                return EXEC_ERROR;
            }

            return EXEC_OK;
        }

        case AST_RUN:
        {
            Value output;

            if (!evaluate_node(
                node,
                environment,
                runtime,
                &output
            ))
                return EXEC_ERROR;

            printf("%s\n", output.string ? output.string : "");
            value_free(&output);
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
            ))
                return EXEC_ERROR;

            switch (value.type) {
                case VALUE_EMPTY:
                    printf("empty\n");
                    break;
                case VALUE_NUMBER:
                    printf("%g\n", value.number);
                    break;
                case VALUE_STRING:
                    printf("%s\n", value.string ? value.string : "");
                    break;
                case VALUE_LIST:
                {
                    printf("[");
                    for (size_t i = 0; i < value.list.count; i++) {
                        if (i > 0)
                            printf(", ");

                        switch (value.list.items[i].type) {
                            case VALUE_EMPTY:
                                printf("empty");
                                break;
                            case VALUE_NUMBER:
                                printf("%g", value.list.items[i].number);
                                break;
                            case VALUE_STRING:
                                printf("%s", value.list.items[i].string ? value.list.items[i].string : "");
                                break;
                            case VALUE_LIST:
                            {
                                char *text = malloc(1);
                                size_t length = 0;
                                size_t capacity = 1;

                                if (text) {
                                    text[0] = '\0';
                                    if (append_value_text(
                                        &text,
                                        &length,
                                        &capacity,
                                        value.list.items[i]
                                    )) {
                                        printf("%s", text);
                                    }
                                    free(text);
                                }
                                break;
                            }
                            case VALUE_BOOLEAN:
                                printf("%s", value.list.items[i].boolean ? "yes" : "no");
                                break;
                        }
                    }
                    printf("]\n");
                    break;
                }
                case VALUE_BOOLEAN:
                    printf("%s\n", value.boolean ? "yes" : "no");
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
            ))
                return EXEC_ERROR;

            int truth = is_truthy(condition);
            value_free(&condition);

            if (truth) {
                for (size_t i = 0; i < node->check.body_count; i++) {
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
            } else if (node->check.otherwise) {
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
            ))
                return EXEC_ERROR;

            if (times.type != VALUE_NUMBER) {
                fprintf(stderr, "Milk error: repeat needs a number\n");
                value_free(&times);
                return EXEC_ERROR;
            }

            int count = (int)times.number;
            value_free(&times);

            if (count < 0)
                count = 0;

            for (int i = 0; i < count; i++) {
                for (size_t j = 0; j < node->repeat.body_count; j++) {
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
            for (;;) {
                Value condition;

                if (!evaluate_node(
                    node->during.condition,
                    environment,
                    runtime,
                    &condition
                ))
                    return EXEC_ERROR;

                int truth = is_truthy(condition);
                value_free(&condition);

                if (!truth)
                    break;

                for (size_t i = 0; i < node->during.body_count; i++) {
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

        case AST_PROGRAM:
            for (size_t i = 0; i < node->program.count; i++) {
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
