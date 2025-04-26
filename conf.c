#include "./conf.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

static const char *config_filepath = NULL;
static const uint8_t FILE_TYPE_VERSION = 1;

static bool cmp_sized_strings(const char *a, size_t a_size, const char *b, size_t b_size) {
    if (a_size != b_size) return false;

    return strncmp(a, b, a_size) == 0;
}

static bool file_exists(const char *filepath) {
    return access(filepath, F_OK) == 0;
}

static FILE *open_config_file(void) {
    if (file_exists(config_filepath)) return fopen(config_filepath, "r+");
    return fopen(config_filepath, "w+");
}

// FILE FORMAT:
// VERSION(uint8_t):EMPTY(uint8_t):CAP(uint64_t):SIZE(uint64_t):KEY(char*):CAP(uint64_t):SIZE(uint64_t):VALUE(char*)
// VERSION(uint8_t):EMPTY(uint8_t):CAP(uint64_t):SIZE(uint64_t):KEY(char*):CAP(uint64_t):SIZE(uint64_t):VALUE(char*)
// VERSION(uint8_t):EMPTY(uint8_t):CAP(uint64_t):SIZE(uint64_t):KEY(char*):CAP(uint64_t):SIZE(uint64_t):VALUE(char*)
// VERSION(uint8_t):EMPTY(uint8_t):CAP(uint64_t):SIZE(uint64_t):KEY(char*):CAP(uint64_t):SIZE(uint64_t):VALUE(char*)

Wodo_Config_Key wodo_config_key_from_cstr(const char *string) {
    return wodo_config_value_from_cstr(string);
}

Wodo_Config_Value wodo_config_value_from_cstr(const char *string) {
    return (Wodo_Config_Value){
        .value = string,
        .size = strlen(string)
    };
}

void wodo_setup_config_file_location(const char *filepath) {
    // TODO: validate filepath
    config_filepath = filepath;
}

uint8_t wodo_set_config(const Wodo_Config_Key key, const Wodo_Config_Value value) {
    typedef struct {
        uint64_t size;
        uint64_t cap;
        char *value;
    } chunk_t;

    if (config_filepath == NULL) {
        return WODO_MISSING_CONFIG_FILE_LOCATION_ERROR_CODE;
    }

    FILE *file = open_config_file();

    if (file == NULL) {
        return WODO_FAIL_OPENING_CONFIG_FILE_ERROR_CODE;
    }

    chunk_t db_key;
    chunk_t db_value;
    uint8_t version;
    uint8_t empty;
    bool equal_key;

    while (true) {
        equal_key = false;
        version = 0;
        empty = 0;
        db_key = (chunk_t){0};
        db_value = (chunk_t){0};

        size_t read_size = fread(&version, sizeof(uint8_t), 1, file);

        if (read_size == 0) break;

        read_size = fread(&empty, sizeof(uint8_t), 1, file);

        if (read_size == 0) {
            fclose(file);

            return WODO_CORRUPTED_CONFIG_FILE_ERROR_CODE;
        }

        if (empty != 0 && empty != 1) {
            fclose(file);

            return WODO_CORRUPTED_CONFIG_FILE_ERROR_CODE;
        }

        if (fread(&db_key.cap, sizeof(uint64_t), 1, file) == 0) {
            fclose(file);

            return WODO_CORRUPTED_CONFIG_FILE_ERROR_CODE;
        }

        if (fread(&db_key.size, sizeof(uint64_t), 1, file) == 0) {
            fclose(file);

            return WODO_CORRUPTED_CONFIG_FILE_ERROR_CODE;
        }

        if (db_key.size > db_key.cap) {
            fclose(file);

            return WODO_CORRUPTED_CONFIG_FILE_ERROR_CODE;
        }

        db_key.value = malloc(sizeof(char) * db_key.size);

        if (fread(db_key.value, sizeof(char), db_key.size, file) != db_key.size) {
            fclose(file);

            return WODO_CORRUPTED_CONFIG_FILE_ERROR_CODE;
        }

        uint64_t key_cap_size_padding = db_key.cap - db_key.size;

        if (key_cap_size_padding > 0) {
            if (fseek(file, (long)key_cap_size_padding, SEEK_CUR) != 0) {
                fclose(file);

                return WODO_CORRUPTED_CONFIG_FILE_ERROR_CODE;
            }
        }

        equal_key = cmp_sized_strings(db_key.value, db_key.size, key.value, key.size);

        if (fread(&db_value.cap, sizeof(uint64_t), 1, file) == 0) {
            fclose(file);

            return WODO_CORRUPTED_CONFIG_FILE_ERROR_CODE;
        }

        if (fread(&db_value.size, sizeof(uint64_t), 1, file) == 0) {
            fclose(file);

            return WODO_CORRUPTED_CONFIG_FILE_ERROR_CODE;
        }

        if (db_value.size > db_value.cap) {
            fclose(file);

            return WODO_CORRUPTED_CONFIG_FILE_ERROR_CODE;
        }

        db_value.value = malloc(sizeof(char) * db_value.size);

        if (fread(db_value.value, sizeof(char), db_value.size, file) != db_value.size) {
            fclose(file);

            return WODO_CORRUPTED_CONFIG_FILE_ERROR_CODE;
        }

        uint64_t value_cap_size_padding = db_value.cap - db_value.size;

        if (value_cap_size_padding > 0) {
            if (fseek(file, (long)value_cap_size_padding, SEEK_CUR) != 0) {
                fclose(file);

                return WODO_CORRUPTED_CONFIG_FILE_ERROR_CODE;
            }
        }

        printf(
            "version: %d, is empty: %s, <key size=%ld cap=%ld value=\"%.*s\"/> <value size=%ld cap=%ld value=\"%.*s\"/>\n",
            version,
            empty == 1 ? "yes" : "no",
            db_key.size,
            db_key.cap,
            (int)db_key.size,
            db_key.value,
            db_value.size,
            db_value.cap,
            (int)db_value.size,
            db_value.value
        );

        // CAP(uint64_t):SIZE(uint64_t):KEY(char*)
        uint64_t key_offset = (sizeof(uint64_t) * 2) + (db_key.cap * sizeof(char));
        // CAP(uint64_t):SIZE(uint64_t):VALUE(char*)
        uint64_t value_offset = (sizeof(uint64_t) * 2) + (db_value.cap * sizeof(char));
        // EMPTY(uint8_t)
        uint64_t empty_offset = sizeof(uint8_t);
        // VERSION(uint8_t)
        uint64_t version_offset = sizeof(uint8_t);
        uint64_t offset = key_offset + value_offset + empty_offset + version_offset;

        // TODO: Insert the config row here
        if (empty == 1) {
            if (db_key.cap >= key.size && db_value.cap >= value.size) {
                if (fseek(file, -((long)offset), SEEK_CUR) != 0) {
                    fclose(file);

                    return WODO_FAIL_UPDATING_KEY_ERROR_CODE;
                }

                equal_key = true;

                break;
            }
        }

        if (!empty && equal_key) {
            if (db_key.cap >= key.size && db_value.cap >= value.size) {
                if (fseek(file, -((long)offset), SEEK_CUR) != 0) {
                    fclose(file);

                    return WODO_FAIL_UPDATING_KEY_ERROR_CODE;
                }

                break;
            } else {
                // VERSION(uint8_t):EMPTY(uint8_t):CAP(uint64_t):SIZE(uint64_t):KEY(char*):CAP(uint64_t):SIZE(uint64_t):VALUE(char*)
                // Here, we are marking this row as unused because there is other rows with same name
                // that is bigger than this one
                if (fseek(file, -((long)(offset - version_offset)), SEEK_CUR) != 0) {
                    fclose(file);

                    return WODO_FAIL_UPDATING_KEY_ERROR_CODE;
                }

                empty = 1;

                fwrite(&empty, sizeof(uint8_t), 1, file);

                uint64_t empty_size = 0;

                fseek(file, sizeof(uint64_t), SEEK_CUR); // jumps capacity, live it as it is
                fwrite(&empty_size, sizeof(uint64_t), 1, file); // update size
                // FILLING "key" value section with zeros
                for (uint64_t i = 0; i < db_key.cap; ++i) {
                    fwrite("0", sizeof(char), 1, file); 
                }
                fseek(file, sizeof(uint64_t), SEEK_CUR); // jumps capacity, live it as it is
                fwrite(&empty_size, sizeof(uint64_t), 1, file); // update size
                // FILLING "value" value section with zeros
                for (uint64_t i = 0; i < db_value.cap; ++i) {
                    fwrite("0", sizeof(char), 1, file); 
                }
            }
        }
    }

    printf(
        "version: %d, is empty: %s, <key size=%ld cap=%ld value=\"%.*s\"/> <value size=%ld cap=%ld value=\"%.*s\"/>\n",
        version,
        empty == 1 ? "yes" : "no",
        db_key.size,
        db_key.cap,
        (int)db_key.size,
        db_key.value,
        db_value.size,
        db_value.cap,
        (int)db_value.size,
        db_value.value
    );

    if (equal_key) {
        printf("overwritting an existing field\n");
    }

    empty = 0;

    fwrite(&FILE_TYPE_VERSION, sizeof(uint8_t), 1, file);
    fwrite(&empty, sizeof(uint8_t), 1, file);

    if (equal_key) {
        fseek(file, sizeof(uint64_t), SEEK_CUR); // jumps capacity, live it as it is
    } else {
        fwrite(&key.size, sizeof(uint64_t), 1, file); // update capacity
    }

    fwrite(&key.size, sizeof(uint64_t), 1, file); // update size
    fwrite(key.value, sizeof(char), key.size, file); 

    if (equal_key) {
        // this check if I have some "unused space" and jump it
        uint64_t key_cap_size_padding = db_key.cap - key.size;

        if (key_cap_size_padding > 0) {
            if (fseek(file, (long)key_cap_size_padding, SEEK_CUR) != 0) {
                fclose(file);

                return WODO_CORRUPTED_CONFIG_FILE_ERROR_CODE;
            }
        }

        fseek(file, sizeof(uint64_t), SEEK_CUR); // jumps capacity, live it as it is
    } else {
        fwrite(&value.size, sizeof(uint64_t), 1, file); // update capacity
    }

    fwrite(&value.size, sizeof(uint64_t), 1, file); // update size
    fwrite(value.value, sizeof(char), value.size, file);

    fclose(file);

    return WODO_OK_CODE;
}

uint8_t wodo_remove_config(const Wodo_Config_Key key);
uint8_t get_config(const Wodo_Config_Key key, Wodo_Config_Value *out);
