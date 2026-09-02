#ifndef MILK_VALUE_H
#define MILK_VALUE_H

#include <stddef.h>

typedef enum {
    VALUE_EMPTY,
    VALUE_NUMBER,
    VALUE_STRING,
    VALUE_LIST,
    VALUE_BOOLEAN
} ValueType;

typedef struct Value Value;

struct Value {
    ValueType type;

    union {
        double number;
        char *string;
        struct {
            Value *items;
            size_t count;
        } list;
        int boolean;
    };
};

Value value_empty(void);
Value value_number(double number);
Value value_string(const char *string);
Value value_list(const Value *items, size_t count);
Value value_copy(const Value *value);
Value value_boolean(int boolean);

void value_free(Value *value);

#endif
