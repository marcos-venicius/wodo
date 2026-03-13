#ifndef _WODO_ARGPARSER_H_
#define _WODO_ARGPARSER_H_

#include <stdio.h>

typedef enum {
    AK_ADD = 1,         // arg1(title)
    AK_ADD_PATH,        // arg1(title) arg2(path)
    AK_REMOVE,          // arg1(path)
    AK_PARSE_AS_JSON,   // arg1(path) tag_filter(-ft) state_filter(-fs)
    AK_LIST,            // tag_filter(-ft) state_filter(-fs)
    AK_FORMAT,          // arg1(path)
    AK_RENAME,          // arg1(path) arg2(title)
    AK_GET_REMINDERS,   //
} ArgumentKind;

typedef struct {
    char **tag_filter;   // CL_ARRAY_INIT
    char **state_filter; // CL_ARRAY_INIT
} Flags;

typedef struct {
    const char *program_name;
    ArgumentKind kind;
    char *arg1;
    char *arg2;

    Flags flags;
} Arguments;

void usage(FILE *stream, const char *program_name, char *error_message, ...);
// returns NULL if has any error
Arguments *parse_arguments(int argc, char **argv);

#endif // !_WODO_ARGPARSER_H_
