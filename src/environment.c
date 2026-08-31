#include "environment.h"

#include <stdlib.h>
#include <string.h>

static char *copy_string(const char *string)
{
    size_t length = strlen(string);

    char *copy =
        malloc(length + 1);

    if (!copy)
        return NULL;

    memcpy(
        copy,
        string,
        length + 1
    );

    return copy;
}

void environment_init(Environment *environment)
{
    environment->variables = NULL;
    environment->count = 0;
    environment->capacity = 0;
}

void environment_free(Environment *environment)
{
    for (
        size_t i = 0;
        i < environment->count;
        i++
    ) {
        free(environment->variables[i].name);

        value_free(
            &environment->variables[i].value
        );
    }

    free(environment->variables);

    environment->variables = NULL;
    environment->count = 0;
    environment->capacity = 0;
}

int environment_set(
    Environment *environment,
    const char *name,
    Value value
)
{
    for (
        size_t i = 0;
        i < environment->count;
        i++
    ) {
        if (
            strcmp(
                environment->variables[i].name,
                name
            ) == 0
        ) {
            value_free(
                &environment->variables[i].value
            );

            environment->variables[i].value =
                value;

            return 1;
        }
    }

    if (
        environment->count >=
        environment->capacity
    ) {
        size_t capacity =
            environment->capacity == 0
                ? 8
                : environment->capacity * 2;

        Variable *variables =
            realloc(
                environment->variables,
                capacity * sizeof(Variable)
            );

        if (!variables)
            return 0;

        environment->variables = variables;
        environment->capacity = capacity;
    }

    char *name_copy =
        copy_string(name);

    if (!name_copy)
        return 0;

    environment->variables[
        environment->count
    ].name = name_copy;

    environment->variables[
        environment->count
    ].value = value;

    environment->count++;

    return 1;
}

int environment_get(
    Environment *environment,
    const char *name,
    Value *value
)
{
    for (
        size_t i = 0;
        i < environment->count;
        i++
    ) {
        if (
            strcmp(
                environment->variables[i].name,
                name
            ) == 0
        ) {
            Variable *variable =
                &environment->variables[i];

            if (
                variable->value.type ==
                VALUE_STRING
            ) {
                *value = value_string(
                    variable->value.string
                );
            } else if (
                variable->value.type ==
                VALUE_NUMBER
            ) {
                *value = value_number(
                    variable->value.number
                );
            } else if (
                variable->value.type ==
                VALUE_BOOLEAN
            ) {
                *value = value_boolean(
                    variable->value.boolean
                );
            } else {
                *value = value_empty();
            }

            return 1;
        }
    }

    return 0;
}
