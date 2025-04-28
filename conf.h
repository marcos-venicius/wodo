#ifndef _WODO_CONF_H_
#define _WODO_CONF_H_

#include <stddef.h>
#include <stdint.h>

#define WODO_OK_CODE 0
#define WODO_MISSING_CONFIG_FILE_LOCATION_ERROR_CODE 1
#define WODO_FAIL_OPENING_CONFIG_FILE_ERROR_CODE 2
#define WODO_CORRUPTED_CONFIG_FILE_ERROR_CODE 3
#define WODO_FAIL_UPDATING_KEY_ERROR_CODE 4
#define WODO_INVALID_ROW_VERSION_ERROR_CODE 5
#define WODO_KEY_NOT_FOUND_ERROR_CODE 6

typedef struct {
    const char *value;
    uint64_t size;
} Wodo_Config_Key;

typedef uint8_t wodo_error_code_t;
typedef Wodo_Config_Key Wodo_Config_Value;

Wodo_Config_Key wodo_config_key_from_cstr(const char *string);
Wodo_Config_Value wodo_config_value_from_cstr(const char *string);
void wodo_setup_config_file_location(const char *filepath);
wodo_error_code_t wodo_set_config(const Wodo_Config_Key key, const Wodo_Config_Value value);
wodo_error_code_t wodo_remove_config(const Wodo_Config_Key key);
wodo_error_code_t wodo_get_config(const Wodo_Config_Key key, Wodo_Config_Value *out);

// TODO: hash the key using sha1 and compress value. so we can standardize the key and prevent large data being stored.
//       allow the user to choose if he want to use this approach.
//
//       The user should be able to compress specific rows, then it'll be saved in the row if the data is compressed.
//       If it is, so, decompress before returning to the user.
//
//       Also allow the user to specify if he wants to use sha1 because sometimes, when using big keys, it may worth it
//       but, if you'll be using small keys may it does not.
//

// TODO: allow the user to reuse the same file descriptor that is currently opened to do sequencial operations
//       so, then it may be more efficient than reopeing the file every single time!
    
// FIXME:
//
// wodo$ ./test add "hello world" "hello guys! how are you?"
// success
// wodo$ ./test add "hello world" "hello guys! how are you? can you answer me?"
// success
// wodo$ ./test add "hello world" "testing"
// success
// wodo$
//
// This error occours because when it finds an empty slot, it doesn't check next rows to see if there is a key with this name
// so, we'll have two of them. I need to think in a clever way to handle this.
    
#endif // _WODO_CONF_H_
