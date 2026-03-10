#define CL_ARRAY_IMPLEMENTATION

#include "./actions.h"
#include <string.h>
#include <errno.h>
#include "./io.h"
#include "./parser.h"
#include "./clibs/arr.h"
#include <stdio.h>
#include "./json.h"
#include "./io.h"

Database global_database; // forward declared?

int add_path_action(const char *name, const char *filepath) {
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

    reset_parser_state();

    wodo_task_t *tasks = parse_tasks(filepath, content, content_size);

    free(content);
    cl_arr_free(tasks);

    database_save(&global_database);

    return 0;
}

int add_action(const char *name) {
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

int remove_action(const char *filepath) {
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

int parse_as_json_action(const char *filepath, Flags flags) {
    (void)flags;

    char *abs_path = realpath(filepath, NULL);

    if (abs_path == NULL) {
        fprintf(stderr, "\033[1;31merror:\033[0m invalid filepath\n");

        return 1;
    }

    char *content;

    size_t length = read_from_file(filepath, &content);

    reset_parser_state();

    wodo_task_t *tasks = parse_tasks(filepath, content, length);

    print_tasks_to_stdout_as_json(tasks, true);

    free(content);
    cl_arr_free(tasks);

    return 0;
}

int list_action() {
    print_database_files_to_stdout_as_json(&global_database);

    return 0;
}

