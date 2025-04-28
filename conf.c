#include "./conf.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#define VERSION_001 1

typedef struct {
    uint64_t size;
    uint64_t cap;
    char *value;
} chunk_t;

typedef struct {
    bool is_empty;
    chunk_t key;
    chunk_t value;
} Version_001_Row;

static const char *config_filepath = NULL;
static const uint8_t CURRENT_VERSION = VERSION_001;
static const uint8_t current_supported_versions[] = {VERSION_001};

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

static long get_file_size(FILE *file) {
    fseek(file, 0, SEEK_END);
    const long file_size = ftell(file);
    rewind(file);

    return file_size;
}

static wodo_error_code_t read_version(FILE *file, uint8_t *version) {
    size_t read_size = fread(version, sizeof(uint8_t), 1, file);

    if (read_size != sizeof(uint8_t)) return WODO_CORRUPTED_CONFIG_FILE_ERROR_CODE;

    for (size_t i = 0; i < sizeof(current_supported_versions) / sizeof(current_supported_versions[0]); ++i) {
        if (*version == current_supported_versions[i]) return WODO_OK_CODE;
    }

    return WODO_INVALID_ROW_VERSION_ERROR_CODE;
}

static wodo_error_code_t read_row_version_001(FILE *file, Version_001_Row **out) {
    uint8_t empty = 0;
    chunk_t db_key = {0};
    chunk_t db_value = {0};

    size_t read_size = fread(&empty, sizeof(uint8_t), 1, file);

    if (read_size != 1 || (empty != 0 && empty != 1)) {
        fclose(file);

        return WODO_CORRUPTED_CONFIG_FILE_ERROR_CODE;
    }

    if (fread(&db_key.cap, sizeof(uint64_t), 1, file) != 1) {
        fclose(file);

        return WODO_CORRUPTED_CONFIG_FILE_ERROR_CODE;
    }

    if (fread(&db_key.size, sizeof(uint64_t), 1, file) != 1) {
        fclose(file);

        return WODO_CORRUPTED_CONFIG_FILE_ERROR_CODE;
    }

    if (db_key.size > db_key.cap) {
        fclose(file);

        return WODO_CORRUPTED_CONFIG_FILE_ERROR_CODE;
    }

    db_key.value = malloc(sizeof(char) * db_key.size);

    if (fread(db_key.value, sizeof(char), db_key.size, file) != db_key.size) {
        free(db_key.value);
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

    if (fread(&db_value.cap, sizeof(uint64_t), 1, file) != 1) {
        fclose(file);

        return WODO_CORRUPTED_CONFIG_FILE_ERROR_CODE;
    }

    if (fread(&db_value.size, sizeof(uint64_t), 1, file) != 1) {
        fclose(file);

        return WODO_CORRUPTED_CONFIG_FILE_ERROR_CODE;
    }

    if (db_value.size > db_value.cap) {
        fclose(file);

        return WODO_CORRUPTED_CONFIG_FILE_ERROR_CODE;
    }

    db_value.value = malloc(sizeof(char) * db_value.size);

    if (fread(db_value.value, sizeof(char), db_value.size, file) != db_value.size) {
        free(db_value.value);
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

    if (out != NULL) {
        *out = malloc(sizeof(Version_001_Row));

        (*out)->key = db_key;
        (*out)->value = db_value;
        (*out)->is_empty = empty == 1;
    }

    return WODO_OK_CODE;
}

static wodo_error_code_t remove_row_version_001(FILE *file, Version_001_Row *row) {
    // CAP(uint64_t):SIZE(uint64_t):KEY(char*)
    uint64_t key_offset = (sizeof(uint64_t) * 2) + (row->key.cap * sizeof(char));
    // CAP(uint64_t):SIZE(uint64_t):VALUE(char*)
    uint64_t value_offset = (sizeof(uint64_t) * 2) + (row->value.cap * sizeof(char));
    // EMPTY(uint8_t)
    uint64_t empty_offset = sizeof(uint8_t);
    // VERSION(uint8_t)
    uint64_t version_offset = sizeof(uint8_t);
    uint64_t offset = key_offset + value_offset + empty_offset + version_offset;

    if (fseek(file, -((long)(offset - version_offset)), SEEK_CUR) != 0) {
        fclose(file);

        return WODO_FAIL_UPDATING_KEY_ERROR_CODE;
    }

    uint8_t empty = 1;

    fwrite(&empty, sizeof(uint8_t), 1, file);

    uint64_t empty_size = 0;

    fseek(file, sizeof(uint64_t), SEEK_CUR); // jumps capacity, live it as it is
    fwrite(&empty_size, sizeof(uint64_t), 1, file); // update size
    // FILLING "key" value section with zeros
    for (uint64_t i = 0; i < row->key.cap; ++i) {
        fwrite("0", sizeof(char), 1, file); 
    }
    fseek(file, sizeof(uint64_t), SEEK_CUR); // jumps capacity, live it as it is
    fwrite(&empty_size, sizeof(uint64_t), 1, file); // update size
    // FILLING "value" value section with zeros
    for (uint64_t i = 0; i < row->value.cap; ++i) {
        fwrite("0", sizeof(char), 1, file); 
    }

    free(row->key.value);
    free(row->value.value);

    return WODO_OK_CODE;
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
    config_filepath = filepath;
}

wodo_error_code_t wodo_set_config(const Wodo_Config_Key key, const Wodo_Config_Value value) {
    if (config_filepath == NULL) {
        return WODO_MISSING_CONFIG_FILE_LOCATION_ERROR_CODE;
    }

    FILE *file = open_config_file();

    if (file == NULL) {
        return WODO_FAIL_OPENING_CONFIG_FILE_ERROR_CODE;
    }

    long file_size = get_file_size(file);

    chunk_t db_key;
    chunk_t db_value;
    uint8_t version = CURRENT_VERSION;
    uint8_t empty = 0;
    bool equal_key = false;

    while (true) {
        if (ftell(file) >= file_size) break;

        equal_key = false;
        version = 0;
        empty = 0;
        db_key = (chunk_t){0};
        db_value = (chunk_t){0};
        uint8_t result_code = 0;

        if ((result_code = read_version(file, &version)) != WODO_OK_CODE) {
            fclose(file);

            return result_code;
        }

        size_t read_size = fread(&empty, sizeof(uint8_t), 1, file);

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

        // CAP(uint64_t):SIZE(uint64_t):KEY(char*)
        uint64_t key_offset = (sizeof(uint64_t) * 2) + (db_key.cap * sizeof(char));
        // CAP(uint64_t):SIZE(uint64_t):VALUE(char*)
        uint64_t value_offset = (sizeof(uint64_t) * 2) + (db_value.cap * sizeof(char));
        // EMPTY(uint8_t)
        uint64_t empty_offset = sizeof(uint8_t);
        // VERSION(uint8_t)
        uint64_t version_offset = sizeof(uint8_t);
        uint64_t offset = key_offset + value_offset + empty_offset + version_offset;

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

    empty = 0;

    fwrite(&CURRENT_VERSION, sizeof(uint8_t), 1, file);
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

wodo_error_code_t wodo_get_config(const Wodo_Config_Key key, Wodo_Config_Value *out) {
    if (config_filepath == NULL) return WODO_MISSING_CONFIG_FILE_LOCATION_ERROR_CODE;

    if (out == NULL) return WODO_OK_CODE;

    FILE *file = open_config_file();

    if (file == NULL) return WODO_FAIL_OPENING_CONFIG_FILE_ERROR_CODE;

    long file_size = get_file_size(file);

    while (true) {
        if (ftell(file) >= file_size) break;

        uint8_t result_code = 0;
        uint8_t version = 0;

        if ((result_code = read_version(file, &version)) != WODO_OK_CODE) {
            fclose(file);

            return result_code;
        }

        switch (version) {
            case VERSION_001: {
                Version_001_Row *row = NULL;

                if ((result_code = read_row_version_001(file, &row)) != WODO_OK_CODE) {
                    return result_code;
                }

                if (row == NULL) break;
                
                if (cmp_sized_strings(row->key.value, row->key.size, key.value, key.size)) {
                    fclose(file);

                    *out = (Wodo_Config_Value){
                        .size = row->value.size,
                        .value = row->value.value
                    };

                    return WODO_OK_CODE;
                }

                free(row->key.value);
                free(row->value.value);
            } break;
            default: return WODO_INVALID_ROW_VERSION_ERROR_CODE;
        }
    }

    fclose(file);

    return WODO_KEY_NOT_FOUND_ERROR_CODE;
}

wodo_error_code_t wodo_remove_config(const Wodo_Config_Key key) {
    if (config_filepath == NULL) return WODO_MISSING_CONFIG_FILE_LOCATION_ERROR_CODE;

    FILE *file = open_config_file();

    if (file == NULL) return WODO_FAIL_OPENING_CONFIG_FILE_ERROR_CODE;

    long file_size = get_file_size(file);
    uint8_t result_code = WODO_OK_CODE;

    while (true) {
        if (ftell(file) >= file_size) break;

        uint8_t version = 0;

        if ((result_code = read_version(file, &version)) != WODO_OK_CODE) {
            fclose(file);

            return result_code;
        }

        switch (version) {
            case VERSION_001: {
                Version_001_Row *row = NULL;

                if ((result_code = read_row_version_001(file, &row)) != WODO_OK_CODE) {
                    return result_code;
                }

                if (row == NULL) break;
                
                if (cmp_sized_strings(row->key.value, row->key.size, key.value, key.size)) {
                    result_code = remove_row_version_001(file, row);

                    goto defer;
                }

                free(row->key.value);
                free(row->value.value);
            } break;
            default: return WODO_INVALID_ROW_VERSION_ERROR_CODE;
        }
    }

defer:
    fclose(file);
    return result_code;
}
