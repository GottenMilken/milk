#include "ast.h"
#include "environment.h"
#include "evaluator.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>

static char *read_file(
    const char *path
)
{
    FILE *file =
        fopen(path, "rb");

    if (!file)
        return NULL;

    fseek(
        file,
        0,
        SEEK_END
    );

    long size =
        ftell(file);

    rewind(file);

    if (size < 0) {
        fclose(file);
        return NULL;
    }

    char *source =
        malloc((size_t)size + 1);

    if (!source) {
        fclose(file);
        return NULL;
    }

    size_t read =
        fread(
            source,
            1,
            (size_t)size,
            file
        );

    fclose(file);

    source[read] = '\0';

    return source;
}

int main(
    int argc,
    char **argv
)
{
    if (argc != 2) {
        fprintf(
            stderr,
            "usage: milk <file>\n"
        );

        return 1;
    }

    char *source =
        read_file(argv[1]);

    if (!source) {
        fprintf(
            stderr,
            "Milk error: could not read '%s'\n",
            argv[1]
        );

        return 1;
    }

    Parser parser;

    parser_init(
        &parser,
        source
    );

    AstNode *program =
        parser_parse(&parser);

    free(source);

    if (!program)
        return 1;

    Environment environment;

    environment_init(
        &environment
    );

    int success =
        evaluator_run(
            program,
            &environment
        );

    environment_free(
        &environment
    );

    ast_free(program);

    return success ? 0 : 1;
}
