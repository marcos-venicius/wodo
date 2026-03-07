#define CL_ARRAY_IMPLEMENTATION

#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "./conf.h"
#include "./database.h"
#include "./io.h"
#include "./utils.h"
#include "./parser.h"
#include "./clibs/arr.h"
#include "./json.h"
#include "./io.h"

int main2() {
    char *content = NULL;

    const char *filename = "./examples/new-format.wodo";

    size_t length = read_from_file(filename, &content);

    wodo_task_t *tasks = parse_tasks(filename, content, length);

    print_tasks_to_stdout_as_json(tasks);

    cl_arr_free(tasks);

    return 0;
}

static Database global_database = {0};
static SysDateTime today_date;
static time_t today_date_timestamp;
static Wodo_Config_Key selected_file_identifier_key = wodo_config_key_from_cstr("selected_file_identifier");

#define defer(code) return_code = code; goto end

static char *shift(int *argc, char ***argv) {
    if (*argc == 0) {
        return NULL;
    }

    (*argc)--;

    return *((*argv)++);
}

static void usage(FILE *stream, const char *program_name, char *error_message, ...) {
    va_list args;

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

static int add_path_action(char *name, char *filepath) {
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

static int add_action(char *name) {
    char *path = get_unix_filepath(name, strlen(name));

    FILE* file = fopen(path, "w");

    if (file == NULL) {
        fprintf(stderr, "\033[1;31merror:\033[0m could not add file due to: %s\n", strerror(errno));

        return 1;
    }

    fclose(file);

    int return_code = add_path_action(name, path);

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

static int parse_as_json_action(char *filepath, Flags flags) {
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

    const char *user_home_folder = get_user_home_folder();
    char *config_file_location = join_paths("%s/%s", user_home_folder, ".wodo/.config");

    wodo_setup_config_file_location(config_file_location);

    database_status_code_t status_code = database_load(&global_database);

    if (status_code != DATABASE_OK_STATUS_CODE) {
        fprintf(stderr, "error: could not open database due to: %s\n", database_status_code_string(status_code));
        return status_code;
    }

    today_date = get_today_date();
    today_date_timestamp = get_timestamp(today_date);

    Arguments args = {0};

    const char *program_name = shift(&argc, &argv);

    char *arg;

    Wodo_Config_Value *out = NULL;

    wodo_error_code_t code = wodo_get_config(selected_file_identifier_key, &out);

    if (code != WODO_OK_CODE && code != WODO_KEY_NOT_FOUND_ERROR_CODE) {
        fprintf(stderr, "\n\033[1;31merror:\033[0m could not get selected file due to %s\n", wodo_error_string(code));

        defer(code);
    }

    bool does_have_selected_file = status_code == DATABASE_OK_STATUS_CODE;

    while ((arg = shift(&argc, &argv)) != NULL) {
        if (arg_cmp(arg, "help", "h")) {
            usage(stdout, program_name, NULL);

            defer(0);
        } else if (arg_cmp(arg, "add", "a")) {
            char *arg1 = shift(&argc, &argv);

            if (arg1 == NULL) {
                usage(stderr, program_name, "action \"%s\" at least a title", arg);

                defer(1);
            }

            char *arg2 = shift(&argc, &argv);

            if (arg2 == NULL) {
                args.kind = AK_ADD;
                args.arg1 = arg1;
            } else {
                args.kind = AK_ADD_PATH;
                args.arg1 = arg1;
                args.arg2 = arg2;
            }
        } else if (arg_cmp(arg, "remove", "r")) {
            char *value = shift(&argc, &argv);

            if (value == NULL && !does_have_selected_file) {
                usage(stderr, program_name, "action \"%s\" expects an id. you don't have any selected files", arg);

                defer(1);
            }

            args.kind = AK_REMOVE;
            args.arg1 = value;
        } else if (arg_cmp(arg, "parse-as-json", "p")) {
            args.kind = AK_PARSE_AS_JSON;

            char *value = shift(&argc, &argv);

            if (value == NULL) {
                usage(stderr, program_name, "action \"%s\" expects a value.", arg);

                defer(1);
            }

            args.arg1 = value;
        } else if (arg_cmp(arg, "list", "l")) {
            args.kind = AK_LIST;
        } else if (arg_cmp(arg, "--filter-tag", "-ft")) {
            char *value = shift(&argc, &argv);

            if (value == NULL) {
                usage(stderr, program_name, "action \"%s\" expects a value.", arg);

                defer(1);
            }

            cl_arr_push(args.flags.tag_filter, value);
        } else if (arg_cmp(arg, "--filter-state", "-fs")) {
            char *value = shift(&argc, &argv);

            if (value == NULL) {
                usage(stderr, program_name, "action \"%s\" expects a value.", arg);

                defer(1);
            }

            cl_arr_push(args.flags.state_filter, value);
        } else {
            if (*arg == '-') {
                usage(stderr, program_name, "flag %s does not exists", arg);
            } else {
                usage(stderr, program_name, "action \"%s\" does not exists", arg);
            }

            defer(1);
        }
    }

    switch (args.kind) {
        case AK_ADD_PATH: return_code = add_path_action(args.arg1, args.arg2); break;
        case AK_ADD: return_code = add_action(args.arg1); break;
        case AK_REMOVE: return_code = remove_action(args.arg1); break;
        case AK_PARSE_AS_JSON: return_code = parse_as_json_action(args.arg1, args.flags); break;
        case AK_LIST: return_code = list_action(); break;
        default: {
            usage(stderr, program_name, NULL);

            defer(1);
        } break;
    }

end:
    free(config_file_location);
    database_free(&global_database);

    return return_code;
}
