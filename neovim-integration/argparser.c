#define CL_ARRAY_IMPLEMENTATION
#include <stdlib.h>
#include <stdarg.h>
#include "./argparser.h"
#include "./utils.h"
#include "./clibs/arr.h"

static char *shift(int *argc, char ***argv) {
    if (*argc == 0) return NULL;

    (*argc)--;

    return *((*argv)++);
}

Arguments *parse_arguments(int argc, char **argv) {
    Arguments *args = calloc(sizeof(Arguments), 1);

    char *arg;

    args->program_name = shift(&argc, &argv);

    while ((arg = shift(&argc, &argv)) != NULL) {
        if (arg_cmp(arg, "help", "h")) {
            usage(stdout, args->program_name, NULL);

            goto error;
        } else if (arg_cmp(arg, "add", "a")) {
            char *arg1 = shift(&argc, &argv);

            if (arg1 == NULL) {
                usage(stderr, args->program_name, "action \"%s\" at least a title", arg);

                goto error;
            }

            char *arg2 = shift(&argc, &argv);

            if (arg2 == NULL) {
                args->kind = AK_ADD;
                args->arg1 = arg1;
            } else {
                args->kind = AK_ADD_PATH;
                args->arg1 = arg1;
                args->arg2 = arg2;
            }
        } else if (arg_cmp(arg, "remove", "r")) {
            char *value = shift(&argc, &argv);

            if (value == NULL) {
                usage(stderr, args->program_name, "action \"%s\" expects an id. you don't have any selected files", arg);

                goto error;
            }

            args->kind = AK_REMOVE;
            args->arg1 = value;
        } else if (arg_cmp(arg, "parse-as-json", "p")) {
            args->kind = AK_PARSE_AS_JSON;

            char *value = shift(&argc, &argv);

            if (value == NULL) {
                usage(stderr, args->program_name, "action \"%s\" expects a value.", arg);

                goto error;
            }

            args->arg1 = value;
        } else if (arg_cmp(arg, "list", "l")) {
            args->kind = AK_LIST;
        } else if (arg_cmp(arg, "--filter-tag", "-ft")) {
            char *value = shift(&argc, &argv);

            if (value == NULL) {
                usage(stderr, args->program_name, "action \"%s\" expects a value.", arg);

                goto error;
            }

            cl_arr_push(args->flags.tag_filter, value);
        } else if (arg_cmp(arg, "--filter-state", "-fs")) {
            char *value = shift(&argc, &argv);

            if (value == NULL) {
                usage(stderr, args->program_name, "action \"%s\" expects a value.", arg);

                goto error;
            }

            cl_arr_push(args->flags.state_filter, value);
        } else {
            if (*arg == '-') {
                usage(stderr, args->program_name, "flag %s does not exists", arg);
            } else {
                usage(stderr, args->program_name, "action \"%s\" does not exists", arg);
            }

            goto error;
        }
    }

    return args;

error:
    free(args);

    return NULL;
}

void usage(FILE *stream, const char *program_name, char *error_message, ...) {
    va_list args;

#ifdef DEV_MODE
    printf("DEVMODE\n");
#endif

    fprintf(stream, "%s [action] [arguments?] [flags?]\n", program_name);
    fprintf(stream, "\n");
    fprintf(stream, "Actions:\n");
    fprintf(stream, "    add             a    [title] [filepath?]   add a new file to the system.\n");
    fprintf(stream, "    remove          r    [filepath]            remove a file from the system.\n");
    fprintf(stream, "    parse-as-json   p    [filepath]            parse a .wodo as json\n");
    fprintf(stream, "    help            h                          display this help message\n");
    fprintf(stream, "    list            l                          list database files\n");
    fprintf(stream, "\n");
    fprintf(stream, "Flags:\n");
    fprintf(stream, "    --filter-tag    -ft                        specify a tag to filter. use this arg multiple times if you want to filter for multiple tags.\n");
    fprintf(stream, "    --filter-state  -fs                        specify a state to filter. use this arg multiple times if you want to filter for multiple states.\n");
    fprintf(stream, "\n");
    fprintf(stream, "States: todo, doing, blocked, done\n");
    fprintf(stream, "\n");
    fprintf(stream, "When you run `add` without a filepath the system will create a new file\n");
    fprintf(stream, "inside the root folder of the application.\n");

    if (error_message != NULL) {
        va_start(args, error_message);
        fprintf(stream, "\n\033[1;31merror: \033[0m");
        vfprintf(stream, error_message, args);
        fprintf(stream, "\n");
        va_end(args);
    }
}
