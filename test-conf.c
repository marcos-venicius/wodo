#include "./conf.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

static char *shift(int *argc, char ***argv) {
    if (*argc == 0) {
        return NULL;
    }

    (*argc)--;

    return *((*argv)++);
}

static inline bool arg_cmp(const char *actual, const char *expected, const char *alternative) {
    return strncmp(actual, expected, strlen(actual)) == 0 || strncmp(actual, alternative, strlen(actual)) == 0;
}

int main(int argc, char **argv) {
    wodo_setup_config_file_location("./settings.wf");

    const char *program_name = shift(&argc, &argv);

    char *arg;

    while ((arg = shift(&argc, &argv)) != NULL) {
        if (arg_cmp(arg, "add", "a")) {
            const char *key = shift(&argc, &argv);
            if (key == NULL) {
                printf("syntax: %s add <key> <value>\n", program_name);

                return 1;
            }
            const char *value = shift(&argc, &argv);
            if (value == NULL) {
                printf("syntax: %s add \"%s\" <value>\n", key, program_name);

                return 1;
            }

            wodo_error_code_t code = wodo_set_config(wodo_config_key_from_cstr(key), wodo_config_value_from_cstr(value));

            switch (code) {
                case WODO_OK_CODE: printf("success\n"); return 0;
                case WODO_MISSING_CONFIG_FILE_LOCATION_ERROR_CODE: printf("missing config file location\n"); return 1;
                case WODO_FAIL_OPENING_CONFIG_FILE_ERROR_CODE: printf("fail opening config file\n"); return 1;
                case WODO_CORRUPTED_CONFIG_FILE_ERROR_CODE: printf("corrupted file\n"); return 1;
                case WODO_FAIL_UPDATING_KEY_ERROR_CODE: printf("fail updating key\n"); return 1;
                case WODO_INVALID_ROW_VERSION_ERROR_CODE: printf("invalid row version\n"); return 1;
                case WODO_KEY_NOT_FOUND_ERROR_CODE: printf("key not found\n"); return 1;
                default: printf("something is really wrong!\n"); return 1;
            }
        } else if (arg_cmp(arg, "get", "g")) {
            const char *key = shift(&argc, &argv);
            if (key == NULL) {
                printf("syntax: %s get <key>\n", program_name);

                return 1;
            }

            Wodo_Config_Value value;

            wodo_error_code_t code = wodo_get_config(wodo_config_key_from_cstr(key), &value);

            switch (code) {
                case WODO_OK_CODE: printf("%.*s\n", (int)value.size, value.value);return 0;
                case WODO_MISSING_CONFIG_FILE_LOCATION_ERROR_CODE: printf("missing config file location\n"); return 1;
                case WODO_FAIL_OPENING_CONFIG_FILE_ERROR_CODE: printf("fail opening config file\n"); return 1;
                case WODO_CORRUPTED_CONFIG_FILE_ERROR_CODE: printf("corrupted file\n"); return 1;
                case WODO_FAIL_UPDATING_KEY_ERROR_CODE: printf("fail updating key\n"); return 1;
                case WODO_INVALID_ROW_VERSION_ERROR_CODE: printf("invalid row version\n"); return 1;
                case WODO_KEY_NOT_FOUND_ERROR_CODE: printf("key not found\n"); return 1;
                default: printf("something is really wrong!\n"); return 1;
            }
        } else if (arg_cmp(arg, "remove", "r")) {
            const char *key = shift(&argc, &argv);

            if (key == NULL) {
                printf("syntax: %s remove <key>\n", program_name);

                return 1;
            }

            wodo_error_code_t code = wodo_remove_config(wodo_config_key_from_cstr(key));

            switch (code) {
                case WODO_OK_CODE: printf("success\n");return 0;
                case WODO_MISSING_CONFIG_FILE_LOCATION_ERROR_CODE: printf("missing config file location\n"); return 1;
                case WODO_FAIL_OPENING_CONFIG_FILE_ERROR_CODE: printf("fail opening config file\n"); return 1;
                case WODO_CORRUPTED_CONFIG_FILE_ERROR_CODE: printf("corrupted file\n"); return 1;
                case WODO_FAIL_UPDATING_KEY_ERROR_CODE: printf("fail updating key\n"); return 1;
                case WODO_INVALID_ROW_VERSION_ERROR_CODE: printf("invalid row version\n"); return 1;
                case WODO_KEY_NOT_FOUND_ERROR_CODE: printf("key not found\n"); return 1;
                default: printf("something is really wrong!\n"); return 1;
            }
        }

        printf("unkown command: %s\n", arg);

        return 1;
    }

    printf("usage: %s <add|get|remove> <key> <value?>\n", program_name);

    return 1;
}
