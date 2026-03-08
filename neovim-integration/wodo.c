#include "argparser.h"
#define CL_ARRAY_IMPLEMENTATION

#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "./database.h"
#include "./io.h"
#include "./parser.h"
#include "./clibs/arr.h"
#include "./json.h"
#include "./io.h"

static Database global_database = {0};

#define defer(code) do { return_code = code; goto end; } while (0)

static int add_path_action(const char *name, const char *filepath) {
    char *abs_path = realpath(filepath, NULL);

    if (abs_path == NULL) {
        fprintf(stderr, "\033[1;31merror:\033[0m invalid filepath\n");

        return 1;
    }

    size_t abs_path_size = strlen(abs_path);
    size_t name_size = strlen(name);

    Database_Db_File *file = malloc(sizeof(Database_Db_File));
    file->deleted = false;
    file->filepath = malloc(abs_path_size + 1);
    file->name = malloc(name_size + 1);

    memcpy(file->filepath, abs_path, strlen(abs_path));
    memcpy(file->name, name, strlen(name));

    file->filepath[abs_path_size] = '\0';
    file->name[name_size] = '\0';

    database_status_code_t status_code = database_add_file(&global_database, file);
    
    if (status_code != DATABASE_OK_STATUS_CODE) {
        fprintf(stderr, "\033[1;31merror:\033[0m could not add file due to: %s\n", database_status_code_string(status_code));

        return status_code;
    }

    char *content = NULL;

    size_t content_size = read_from_file(abs_path, &content);

    wodo_task_t *tasks = parse_tasks(filepath, content, content_size);

    cl_arr_free(tasks);

    free(content);
    database_save(&global_database);

    return 0;
}

static int add_action(const char *name) {
    char *path = get_unix_filepath(name, strlen(name));

    FILE* file = fopen(path, "w");

    if (file == NULL) {
        fprintf(stderr, "\033[1;31merror:\033[0m could not add file due to: %s\n", strerror(errno));

        return 1;
    }

    fclose(file);

    int return_code = add_path_action(name, path);

    printf("%s\n", path);

    free(path);

    return return_code;
}

static int remove_action(const char *filepath) {
    database_status_code_t status_code;

    Database_Db_File *file = NULL;

    status_code = database_get_file_by_filepath(&file, &global_database, filepath);

    if (status_code == DATABASE_NOT_FOUND_STATUS_CODE) {
        return 0;
    }

    if (status_code != DATABASE_OK_STATUS_CODE) {
        return status_code;
    }

    if (file == NULL) return 0;

    database_delete_file(file);

    database_save(&global_database);

    return 0;
}

static int parse_as_json_action(const char *filepath, Flags flags) {
    (void)flags;

    char *abs_path = realpath(filepath, NULL);

    if (abs_path == NULL) {
        fprintf(stderr, "\033[1;31merror:\033[0m invalid filepath\n");

        return 1;
    }

    char *content;

    size_t length = read_from_file(filepath, &content);

    wodo_task_t *tasks = parse_tasks(filepath, content, length);

    print_tasks_to_stdout_as_json(tasks);

    cl_arr_free(tasks);

    return 0;
}

static int list_action() {
    print_database_files_to_stdout_as_json(&global_database);

    return 0;
}

int main(int argc, char **argv) {
    int return_code = 0;

    database_status_code_t status_code = database_load(&global_database);

    if (status_code != DATABASE_OK_STATUS_CODE) {
        fprintf(stderr, "error: could not open database due to: %s\n", database_status_code_string(status_code));
        return status_code;
    }

    Arguments *args = parse_arguments(argc, argv);

    if (args == NULL)
        defer(1);

    switch (args->kind) {
        case AK_ADD_PATH: return_code = add_path_action(args->arg1, args->arg2); break;
        case AK_ADD: return_code = add_action(args->arg1); break;
        case AK_REMOVE: return_code = remove_action(args->arg1); break;
        case AK_PARSE_AS_JSON: return_code = parse_as_json_action(args->arg1, args->flags); break;
        case AK_LIST: return_code = list_action(); break;
        default: {
            usage(stderr, args->program_name, NULL);

            defer(1);
        } break;
    }

end:
    if (args != NULL) free(args);

    database_free(&global_database);

    return return_code;
}
