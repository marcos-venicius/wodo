#define CL_ARRAY_IMPLEMENTATION

#include <string.h>
#include <errno.h>
#include "./io.h"
#include <stdio.h>
#include "./json.h"
#include "./io.h"
#include "./database.h"
#include "visualizer.h"
#include "./parser.h"
#include "./actions.h"
#include "utils.h"
#include "./clibs/arr.h"
#include "./crossplatformops.h"

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

    database_status_code_t status_code = database_add_file(file);
    
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

    database_save();

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

    status_code = database_get_file_by_filepath(&file, filepath);

    if (status_code == DATABASE_NOT_FOUND_STATUS_CODE) {
        return 0;
    }

    if (status_code != DATABASE_OK_STATUS_CODE) {
        return status_code;
    }

    if (file == NULL) return 0;

    database_delete_file(file);

    database_save();

    return 0;
}

int parse_as_json_action(const char *filepath, Flags flags) {
    char *abs_path = realpath(filepath, NULL);

    if (abs_path == NULL) {
        fprintf(stderr, "\033[1;31merror:\033[0m invalid filepath\n");

        return 1;
    }

    database_status_code_t status_code = DATABASE_OK_STATUS_CODE;

    if ((status_code = database_get_file_by_filepath(NULL, abs_path)) != DATABASE_OK_STATUS_CODE) {
        fprintf(stderr, "error: could not format file because we couldn't get it from database: %s\n", database_status_code_string(status_code));
        return status_code;
    }

    char *content;

    size_t length = read_from_file(filepath, &content);

    reset_parser_state();

    wodo_task_t *tasks = parse_tasks(filepath, content, length);

    print_tasks_to_stdout_as_json(tasks, default_task_predicate, &flags);

    printf("\n");

    free(content);
    cl_arr_free(tasks);

    return 0;
}

int list_action(Flags flags) {
    print_database_files_to_stdout_as_json(default_task_predicate, &flags);

    return 0;
}

static void print_trimed_string(wodo_string_t string) {
    size_t start_cursor = 0;
    size_t end_cursor = string.length - 1;

    while (start_cursor < string.length && string.value[start_cursor] == ' ') start_cursor++;
    while (end_cursor > start_cursor && string.value[end_cursor] == ' ') end_cursor--;

    printf("%.*s", (int)(end_cursor - start_cursor + 1), string.value + start_cursor);
}

static void print_trimed_line_string(wodo_string_t string) {
    size_t first_line = 0;
    size_t last_line = string.length - 1;
    size_t start_cursor = 0;
    size_t end_cursor = string.length - 1;

    while (true) {
        while (start_cursor < string.length && string.value[start_cursor] == ' ') start_cursor++;

        // reached the end of the description with all blank lines
        if (start_cursor >= string.length) {
            first_line = string.length - 1;

            break;
        }

        if (string.value[start_cursor] == '\n') {
            start_cursor++;
            continue;
        }

        first_line = start_cursor;

        break;
    }

    while (true) {
        while (end_cursor > start_cursor && string.value[end_cursor] == ' ') end_cursor--;

        // reached the beginning of the description with all blank lines
        if (end_cursor <= start_cursor) {
            last_line = start_cursor;

            break;
        }

        if (string.value[end_cursor] == '\n') {
            end_cursor--;
            continue;
        }

        last_line = end_cursor;

        break;
    }

    printf("%.*s", (int)(last_line - first_line + 1), string.value + first_line);
}

int format_action(const char *filepath) {
    char *abs_path = realpath(filepath, NULL);

    if (abs_path == NULL) {
        fprintf(stderr, "\033[1;31merror:\033[0m invalid filepath\n");

        return 1;
    }

    database_status_code_t status_code = DATABASE_OK_STATUS_CODE;

    if ((status_code = database_get_file_by_filepath(NULL, abs_path)) != DATABASE_OK_STATUS_CODE) {
        fprintf(stderr, "error: could not format file because we couldn't get it from database: %s\n", database_status_code_string(status_code));
        return status_code;
    }

    char *content;

    size_t length = read_from_file(filepath, &content);

    reset_parser_state();

    wodo_task_t *tasks = parse_tasks(filepath, content, length);

    for (size_t i = 0; i < cl_arr_len(tasks); i++) {
        if (i > 0) printf("\n");

        wodo_task_t task = tasks[i];

        printf("%% ");
        print_trimed_string(task.title.string);
        printf("\n");
        printf("\n");
        printf(".state   ");
        switch (task.state_property.state) {
            case Wodo_Task_State_Todo: printf("todo\n"); break;
            case Wodo_Task_State_Doing: printf("doing\n"); break;
            case Wodo_Task_State_Blocked: printf("blocked\n"); break;
            case Wodo_Task_State_Done: printf("done\n"); break;
            default: assert(0 && "unhandled state during formatting");
        }
        printf(".date    ");
        print_wodo_datetime(task.date_property.datetime, false);
        printf("\n");
        printf(".tags");

        if (cl_arr_len(task.tags_property.node_array) > 0) {
            printf("    ");
            for (size_t j = 0; j < (cl_arr_len(task.tags_property.node_array)); j++) {
                if (j > 0) printf(" ");

                wodo_string_t tag = task.tags_property.node_array[j].string;

                printf("%.*s", (int)tag.length, tag.value);
            }
        }
        printf("\n");

        if (task.remind_property.boolean) {
            printf(".remind\n");
        }

        if (task.description.string.length > 0) {
            printf("\n");
            print_trimed_line_string(task.description.string);
            printf("\n");
        }
    }

    return 0;
}

int rename_action(const char *filepath, char *title) {
    char *abs_path = realpath(filepath, NULL);

    if (abs_path == NULL) {
        fprintf(stderr, "\033[1;31merror:\033[0m invalid filepath\n");

        return 1;
    }

    for (size_t i = 0; i < cl_arr_len(global_database.files); i++) {
        Database_Db_File *file = global_database.files[i];

        if (strncmp(file->filepath, filepath, strlen(file->filepath)) == 0) {
            database_rename_file(file, title);

            database_save();

            return 0;
        }
    }

    return DATABASE_NOT_FOUND_STATUS_CODE;
}

static bool get_reminders_action_task_predicate(wodo_task_t task, Flags *flags) {
    (void)flags;

    return task.state_property.state != Wodo_Task_State_Done && task.remind_property.boolean;
}

int get_reminders_action() {
    print_database_files_to_stdout_as_json(get_reminders_action_task_predicate, NULL);

    return 0;
}

int init_repository_action() {
    char base_path_buffer[FILENAME_MAX];

    if (GetCurrentDir(base_path_buffer, sizeof(base_path_buffer)) == NULL) {
        fprintf(stderr, "error: could not get current working directory: %s\n", strerror(errno));

        return DATABASE_ERRNO;
    }

    if (has_repository_at(base_path_buffer)) {
        printf("You already have a Wodo repository at %s\n", base_path_buffer);

        return DATABASE_OK_STATUS_CODE;
    }

    char *initialized_dir = database_init(base_path_buffer);

    printf("Initialized empty Wodo repository in %s\n", initialized_dir);

    return DATABASE_OK_STATUS_CODE;
}
