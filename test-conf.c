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

            printf("result: %s\n", wodo_error_string(code));

            if (code != WODO_OK_CODE) return 1;

            return 0;
        } else if (arg_cmp(arg, "get", "g")) {
            const char *key = shift(&argc, &argv);
            if (key == NULL) {
                printf("syntax: %s get <key>\n", program_name);

                return 1;
            }

            Wodo_Config_Value value;

            wodo_error_code_t code = wodo_get_config(wodo_config_key_from_cstr(key), &value);

            printf("result: %s\n", wodo_error_string(code));

            if (code != WODO_OK_CODE) return 1;

            printf("value: \"%.*s\"\n", (int)value.size, value.value);
            return 0;
        } else if (arg_cmp(arg, "remove", "r")) {
            const char *key = shift(&argc, &argv);

            if (key == NULL) {
                printf("syntax: %s remove <key>\n", program_name);

                return 1;
            }

            wodo_error_code_t code = wodo_remove_config(wodo_config_key_from_cstr(key));

            printf("result: %s\n", wodo_error_string(code));

            if (code != WODO_OK_CODE) return 1;

            return 0;
        }

        printf("unkown command: %s\n", arg);

        return 1;
    }

    printf("usage: %s <add|get|remove> <key> <value?>\n", program_name);

    return 1;
}
