#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "database.h"
#include "actions.h"

#define defer(code) do { return_code = code; goto end; } while (0)

/*
 * I have an extern inside "database.h" file.
 **/
Database global_database = {0};

int main(int argc, char **argv) {
    int return_code = 0;

    Arguments *args = parse_arguments(argc, argv);

    if (args == NULL)
        defer(1);

    if (args->kind == AK_INIT) {
        defer(init_repository_action());
    }

    database_status_code_t status_code;

    if ((status_code = load_wodo_database_working_directory()) != DATABASE_OK_STATUS_CODE) {
        fprintf(stderr, "error: %s\n", database_status_code_string(status_code));

        return status_code;
    }

    if ((status_code = database_load()) != DATABASE_OK_STATUS_CODE) {
        fprintf(stderr, "error: %s\n", database_status_code_string(status_code));
        return status_code;
    }

    switch (args->kind) {
        case AK_ADD: return_code = add_action(args->arg1); break;
        case AK_REMOVE: return_code = remove_action(args->arg1); break;
        case AK_PARSE_AS_JSON: return_code = parse_as_json_action(args->arg1, args->flags); break;
        case AK_LIST: return_code = list_action(args->flags); break;
        case AK_FORMAT: return_code = format_action(args->arg1); break;
        case AK_RENAME: return_code = rename_action(args->arg1, args->arg2); break;
        case AK_GET_REMINDERS: return_code = get_reminders_action(); break;
        default: {
            usage(stderr, args->program_name, "invalid command line options");

            defer(1);
        } break;
    }

end:
    if (args != NULL) free(args);

    database_free();

    return return_code;
}
