#ifndef MILK_ENVIRONMENT_H
#define MILK_ENVIRONMENT_H

#include <stddef.h>

#include "value.h"

typedef struct {
    char *name;
    Value value;
} Variable;

typedef struct Environment Environment;

struct Environment {
    Variable *variables;
    size_t count;
    size_t capacity;
    Environment *parent;
};

void environment_init(Environment *environment);
void environment_init_child(
    Environment *environment,
    Environment *parent
);
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
