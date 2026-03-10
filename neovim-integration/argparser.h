#ifndef _WODO_ARGPARSER_H_
#define _WODO_ARGPARSER_H_

#include <stdio.h>
typedef enum {
    AK_ADD = 1,
    AK_ADD_PATH,
    AK_REMOVE,
    AK_PARSE_AS_JSON,
    AK_LIST,
    AK_FORMAT,
} ArgumentKind;

typedef struct {
    char **tag_filter; // CL_ARRAY_INIT
    char **state_filter; // CL_ARRAY_INIT
} Flags;

typedef struct {
    const char *program_name;
    ArgumentKind kind;
    const char *arg1;
    const char *arg2;

    Flags flags;
} Arguments;

void usage(FILE *stream, const char *program_name, char *error_message, ...);
// returns NULL if has any error
Arguments *parse_arguments(int argc, char **argv);

#endif // !_WODO_ARGPARSER_H_
