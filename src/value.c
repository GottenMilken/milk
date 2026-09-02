#include "value.h"

#include <stdlib.h>
#include <string.h>

static char *copy_string(const char *string)
{
    if (!string)
        return NULL;

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
    value.string = copy_string(string ? string : "");
    return value;
}

Value value_list(const Value *items, size_t count)
{
    Value value;

    value.type = VALUE_LIST;
    value.list.items = NULL;
    value.list.count = 0;

    if (count == 0)
        return value;

    value.list.items =
        calloc(count, sizeof(Value));

    if (!value.list.items) {
        value.type = VALUE_EMPTY;
        return value;
    }

    for (size_t i = 0; i < count; i++) {
        value.list.items[i] =
            value_copy(&items[i]);

        if (
            items[i].type != VALUE_EMPTY &&
            value.list.items[i].type == VALUE_EMPTY
        ) {
            for (size_t j = 0; j < i; j++)
                value_free(&value.list.items[j]);
            free(value.list.items);
            value.list.items = NULL;
            return value_empty();
        }
    }

    value.list.count = count;
    return value;
}

Value value_copy(const Value *value)
{
    if (!value)
        return value_empty();

    switch (value->type) {
        case VALUE_EMPTY:
            return value_empty();

        case VALUE_NUMBER:
            return value_number(value->number);

        case VALUE_STRING:
            return value_string(value->string);

        case VALUE_LIST:
            return value_list(
                value->list.items,
                value->list.count
            );

        case VALUE_BOOLEAN:
            return value_boolean(value->boolean);
    }

    return value_empty();
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

    if (value->type == VALUE_STRING) {
        free(value->string);
    } else if (value->type == VALUE_LIST) {
        for (size_t i = 0; i < value->list.count; i++)
            value_free(&value->list.items[i]);
        free(value->list.items);
    }

    value->type = VALUE_EMPTY;
    value->list.items = NULL;
    value->list.count = 0;
}
