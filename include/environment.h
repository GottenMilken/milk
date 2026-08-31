#ifndef MILK_ENVIRONMENT_H
#define MILK_ENVIRONMENT_H

#include <stddef.h>

#include "value.h"

typedef struct {
    char *name;
    Value value;
} Variable;

typedef struct {
    Variable *variables;
    size_t count;
    size_t capacity;
} Environment;

void environment_init(Environment *environment);
void environment_free(Environment *environment);

int environment_set(
    Environment *environment,
    const char *name,
    Value value
);

int environment_get(
    Environment *environment,
    const char *name,
    Value *value
);

#endif
