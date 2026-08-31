#ifndef MILK_VALUE_H
#define MILK_VALUE_H

typedef enum {
    VALUE_EMPTY,
    VALUE_NUMBER,
    VALUE_STRING,
    VALUE_BOOLEAN
} ValueType;

typedef struct {
    ValueType type;

    union {
        double number;
        char *string;
        int boolean;
    };
} Value;

Value value_empty(void);
Value value_number(double number);
Value value_string(const char *string);
Value value_boolean(int boolean);

void value_free(Value *value);

#endif
