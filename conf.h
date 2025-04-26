#ifndef _WODO_CONF_H_
#define _WODO_CONF_H_

#include <stddef.h>
#include <stdint.h>

#define WODO_OK_CODE 0
#define WODO_MISSING_CONFIG_FILE_LOCATION_ERROR_CODE 1
#define WODO_FAIL_OPENING_CONFIG_FILE_ERROR_CODE 2
#define WODO_CORRUPTED_CONFIG_FILE_ERROR_CODE 3
#define WODO_FAIL_UPDATING_KEY_ERROR_CODE 4

typedef struct {
    const char *value;
    uint64_t size;
} Wodo_Config_Key;

typedef Wodo_Config_Key Wodo_Config_Value;

Wodo_Config_Key wodo_config_key_from_cstr(const char *string);
Wodo_Config_Value wodo_config_value_from_cstr(const char *string);
void wodo_setup_config_file_location(const char *filepath);
uint8_t wodo_set_config(const Wodo_Config_Key key, const Wodo_Config_Value value);
uint8_t wodo_remove_config(const Wodo_Config_Key key);
uint8_t get_config(const Wodo_Config_Key key, Wodo_Config_Value *out);

#endif // _WODO_CONF_H_
