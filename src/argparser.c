#define CL_ARRAY_IMPLEMENTATION
#include <stdlib.h>
#include <stdarg.h>
#include "argparser.h"
#include "utils.h"
#include "arr.h"

static char *shift(int *argc, char ***argv) {
    if (*argc == 0) return NULL;

    (*argc)--;

    return *((*argv)++);
}

#define getarg() shift(&argc, &argv)

Arguments *parse_arguments(int argc, char **argv) {
    Arguments *args = calloc(sizeof(Arguments), 1);

    args->flags.state_filter = NULL;
    args->flags.tag_filter = NULL;

    char *arg;

    args->program_name = getarg();

    while ((arg = getarg()) != NULL) {
        if (arg_cmp(arg, "help", "h")) {
            usage(stdout, args->program_name, NULL);

            goto error;
        } else if (arg_cmp(arg, "add", "a")) {
            char *arg1 = getarg();

            if (arg1 == NULL) {
                usage(stderr, args->program_name, "action \"%s\" expects a title", arg);

                goto error;
            }

            args->kind = AK_ADD;
            args->arg1 = arg1;
        } else if (arg_cmp(arg, "remove", "r")) {
            char *value = getarg();

            if (value == NULL) {
                usage(stderr, args->program_name, "action \"%s\" expects a path.", arg);

                goto error;
            }

            args->kind = AK_REMOVE;
            args->arg1 = value;
        } else if (arg_cmp(arg, "rename", "n")) {
            char *path = getarg();

            if (path == NULL) {
                usage(stderr, args->program_name, "action \"%s\" expects a path.", arg);

                goto error;
            }

            char *title = getarg();

            if (title == NULL) {
                usage(stderr, args->program_name, "action \"%s\" expects a title.", arg);

                goto error;
            }

            args->kind = AK_RENAME;
            args->arg1 = path;
            args->arg2 = title;
        } else if (arg_cmp(arg, "parse", "p")) {
            args->kind = AK_PARSE;

            char *value = getarg();

            if (value == NULL) {
                usage(stderr, args->program_name, "action \"%s\" expects a value.", arg);

                goto error;
            }

            args->arg1 = value;
        } else if (arg_cmp(arg, "list", "l")) {
            args->kind = AK_LIST;
        } else if (arg_cmp(arg, "format", "f")) {
            args->kind = AK_FORMAT;

            char *value = getarg();

            if (value == NULL) {
                usage(stderr, args->program_name, "action \"%s\" expects a value.", arg);

                goto error;
            }

            args->arg1 = value;
        } else if (arg_cmp_single(arg, "reminders")) {
            args->kind = AK_GET_REMINDERS;
        } else if (arg_cmp_single(arg, "init")) {
            args->kind = AK_INIT;
        } else if (arg_cmp(arg, "--filter-tag", "-ft")) {
            char *value = getarg();

            if (value == NULL) {
                usage(stderr, args->program_name, "action \"%s\" expects a value.", arg);

                goto error;
            }

            cl_arr_push(args->flags.tag_filter, value);
        } else if (arg_cmp(arg, "--filter-state", "-fs")) {
            char *value = getarg();

            if (value == NULL) {
                usage(stderr, args->program_name, "action \"%s\" expects a value.", arg);

                goto error;
            }

            cl_arr_push(args->flags.state_filter, value);
        } else {
            if (*arg == '-') {
                usage(stderr, args->program_name, "flag %s does not exists.", arg);
            } else {
                usage(stderr, args->program_name, "action \"%s\" does not exists.", arg);
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

    fprintf(stream, "Usage: %s [action] [arguments] [flags]\n\n", program_name);

    // --- SYSTEM MANAGEMENT ---
    fprintf(stream, "System management:\n");
    fprintf(stream, "  init                          Init a repository\n\n");

    // --- FILE MANAGEMENT GROUP ---
    fprintf(stream, "File Management:\n");
    fprintf(stream, "  add, a     <title>            Add a new .wodo file\n");
    fprintf(stream, "  remove, r  <path>             Remove a file from the system\n");
    fprintf(stream, "  rename, n  <path> <title>     Rename an existing .wodo file\n");
    fprintf(stream, "  format, f  <path>             Format and clean a .wodo file from stdin;\n");
    fprintf(stream, "                                <path> here is used only for error reporting.\n\n");

    // --- DATA & INSPECTION GROUP ---
    fprintf(stream, "Data & Inspection:\n");
    fprintf(stream, "  reminders                     List all (not done) tasks marked with 'remind' property\n");
    fprintf(stream, "  list, l    [flags]            List all database files\n");
    fprintf(stream, "  parse, p   <path> [flags]     Parse a .wodo file;\n");
    fprintf(stream, "                                <path> here is used only for error reporting.\n\n");

    // --- FILTER FLAGS GROUP ---
    fprintf(stream, "Global Filter Flags (use with list/parse):\n");
    fprintf(stream, "  -ft, --filter-tag   <tag>     Filter by tag (can be used multiple times)\n");
    fprintf(stream, "  -fs, --filter-state <state>   Filter by state (can be used multiple times)\n\n");

    // --- REFERENCE DATA ---
    fprintf(stream, "Available States:\n");
    fprintf(stream, "  todo, doing, blocked, done\n\n");

    fprintf(stream, "General:\n");
    fprintf(stream, "  help, h                       Display this help message\n");

    if (error_message != NULL) {
        va_start(args, error_message);
        fprintf(stream, "\n\033[1;31merror: \033[0m");
        vfprintf(stream, error_message, args);
        fprintf(stream, "\n");
        va_end(args);
    }
}
