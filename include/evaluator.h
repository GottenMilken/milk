#ifndef MILK_EVALUATOR_H
#define MILK_EVALUATOR_H

#include "ast.h"
#include "environment.h"

int evaluator_run(
    AstNode *program,
    Environment *environment
);

#endif
