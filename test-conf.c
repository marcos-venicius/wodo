#include "./conf.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>

int main(void) {
    wodo_setup_config_file_location("./settings");

    Wodo_Config_Key key = wodo_config_key_from_cstr("hello world");

    uint8_t result = wodo_set_config(key, wodo_config_value_from_cstr("lorem ipsum dolor sit ammet consectur"));

    switch (result) {
        case WODO_OK_CODE: printf("success\n"); break;
        case WODO_MISSING_CONFIG_FILE_LOCATION_ERROR_CODE: printf("missing config file location\n"); break;
        case WODO_FAIL_OPENING_CONFIG_FILE_ERROR_CODE: printf("fail to open config file\n"); break;
        case WODO_CORRUPTED_CONFIG_FILE_ERROR_CODE: printf("corrupted file\n"); break;
        case WODO_FAIL_UPDATING_KEY_ERROR_CODE: printf("failed updating key due to: %s\n", strerror(errno)); break;
        default: printf("what? %d\n", result);
    }

    Wodo_Config_Value value;

    if ((result = wodo_get_config(key, &value)) != WODO_OK_CODE) {
        switch (result) {
            case WODO_OK_CODE: printf("success\n"); break;
            case WODO_MISSING_CONFIG_FILE_LOCATION_ERROR_CODE: printf("missing config file location\n"); break;
            case WODO_FAIL_OPENING_CONFIG_FILE_ERROR_CODE: printf("fail to open config file\n"); break;
            case WODO_CORRUPTED_CONFIG_FILE_ERROR_CODE: printf("corrupted file\n"); break;
            case WODO_FAIL_UPDATING_KEY_ERROR_CODE: printf("failed updating key due to: %s\n", strerror(errno)); break;
            default: printf("what? %d\n", result);
        }
        return 1;
    }

    printf("value: \"%.*s\"\n", (int)value.size, value.value);

    wodo_remove_config(key);

    if ((result = wodo_get_config(key, &value)) != WODO_KEY_NOT_FOUND_ERROR_CODE) {
        switch (result) {
            case WODO_OK_CODE: printf("success\n"); break;
            case WODO_MISSING_CONFIG_FILE_LOCATION_ERROR_CODE: printf("missing config file location\n"); break;
            case WODO_FAIL_OPENING_CONFIG_FILE_ERROR_CODE: printf("fail to open config file\n"); break;
            case WODO_CORRUPTED_CONFIG_FILE_ERROR_CODE: printf("corrupted file\n"); break;
            case WODO_FAIL_UPDATING_KEY_ERROR_CODE: printf("failed updating key due to: %s\n", strerror(errno)); break;
            default: printf("what? %d\n", result);
        }
        return 1;
    }

    return 0;
}
