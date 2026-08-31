#include "value.h"

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

Value value_empty(void)
{
    Value value;

    value.type = VALUE_EMPTY;

    return value;
}

Value value_number(double number)
{
    Value value;

    value.type = VALUE_NUMBER;
    value.number = number;

    return value;
}

Value value_string(const char *string)
{
    Value value;

    value.type = VALUE_STRING;
    value.string = copy_string(string);

    return value;
}

Value value_boolean(int boolean)
{
    Value value;

    value.type = VALUE_BOOLEAN;
    value.boolean = boolean;

    return value;
}

void value_free(Value *value)
{
    if (!value)
        return;

    if (value->type == VALUE_STRING)
        free(value->string);

    value->type = VALUE_EMPTY;
}
