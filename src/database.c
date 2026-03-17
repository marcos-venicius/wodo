#define CL_ARRAY_IMPLEMENTATION
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <libgen.h>
#include "database.h"
#include "utils.h"
#include "arr.h"
#include "crypt.h"
#include "crossplatformops.h"


#ifdef DEV_MODE
static const char *db_folder_name = ".wodo.dev";
#else
static const char *db_folder_name = ".wodo";
#endif

static const char *file_extension = ".wodo";
static const char *db_filename = ".wodo.db";
static const char *db_file_magic_bytes = ".WODO";
static char *wodo_current_working_directory = NULL;
static char *wodo_current_working_directory_db = NULL;

static Database_File_Path get_unix_filepath(const char *name, size_t name_size) {
    unsigned char *hash = hash_bytes(name, name_size);
    unsigned long timestamp = get_current_timestamp();
    char timestamp_string[32];

    snprintf(timestamp_string, sizeof(timestamp_string), "%lu", timestamp);

    char *relative_filepath = join_paths("%s-%s%s", hash, timestamp_string, file_extension);
    char *absolute_filepath = join_paths("%s/%s", wodo_current_working_directory, relative_filepath);

    free(hash);

    return (Database_File_Path){
        .relative = relative_filepath,
        .absolute = absolute_filepath
    };
}

static void save_database_latest_version(FILE *file) {
    short version = DBV_2;

    // magic bytes + \0
    fwrite(db_file_magic_bytes, sizeof(char), strlen(db_file_magic_bytes) + 1, file);
    // database version
    fwrite(&version, sizeof(short), 1, file);

    size_t length = 0;

    for (size_t i = 0; i < cl_arr_len(global_database.files); i++) {
        Database_File *it = global_database.files[i];

        if (it->view_deleted) continue;

        length++;
    }

    fwrite(&length, sizeof(uint64_t), 1, file);

    for (size_t i = 0; i < cl_arr_len(global_database.files); i++) {
        Database_File *it = global_database.files[i];

        if (it->view_deleted) continue;
        uint64_t name_size = strlen(it->name) + 1; // needs to save the null terminator
        uint64_t path_size = strlen(it->relative_filepath) + 1; // needs to save the null terminator

        fwrite(&name_size, sizeof(uint64_t), 1, file);
        fwrite(it->name, sizeof(char), name_size, file);
        fwrite(&path_size, sizeof(uint64_t), 1, file);
        fwrite(it->relative_filepath, sizeof(char), path_size, file);
    }
}

static void database_save() {
    FILE *file = fopen(wodo_current_working_directory_db, "wb");

    if (file == NULL) {
        fprintf(stderr, "could not open database file: %s\n", strerror(errno));
        exit(1);
    }

    switch (global_database.version) {
        case DBV_0: save_database_latest_version(file); break;
        case DBV_1: save_database_latest_version(file); break;
        case DBV_2: save_database_latest_version(file); break;
        default: fprintf(stderr, "error: could not handle database version %d\n", global_database.version); exit(1);
    }

    fclose(file);
}

bool has_repository_at(const char *base_path) {
    char *wodo_folder = join_paths("%s/%s", base_path, db_folder_name);

    return file_exists(wodo_folder, true);
}

database_status_code_t load_wodo_database_working_directory() {
    char current_dir_buffer[FILENAME_MAX];

    if (GetCurrentDir(current_dir_buffer, sizeof(current_dir_buffer)) == NULL) {
        return DATABASE_WODO_FOLDER_NOT_FOUND;
    }

    char *base_path = strdup(current_dir_buffer);

    while (true) {
        char *wodo_folder = join_paths("%s/%s", base_path, db_folder_name);

        if (file_exists(wodo_folder, true)) {
            wodo_current_working_directory = wodo_folder;
            wodo_current_working_directory_db = join_paths("%s/%s", wodo_folder, db_filename);

            return DATABASE_OK_STATUS_CODE;
        }

        free(wodo_folder);

        size_t previous_length = strlen(base_path);

        base_path = dirname(base_path);

        size_t current_length = strlen(base_path);

        if (previous_length == current_length) {
            free(base_path);

            return DATABASE_WODO_FOLDER_NOT_FOUND;
        }
    }

    return DATABASE_WODO_FOLDER_NOT_FOUND;
}

static void free_db_file(Database_File *file) {
    if (file->name != NULL) {
        free(file->name); 
        file->name = NULL;
    }

    if (file->view_absolute_filepath != NULL) {
        free(file->view_absolute_filepath);
        file->view_absolute_filepath = NULL;
    }

    if (file->relative_filepath != NULL) {
        free(file->relative_filepath);
        file->relative_filepath = NULL;
    }

    free(file);
}

const char *database_status_code_string(database_status_code_t status_code) {
    switch (status_code) {
        case DATABASE_OK_STATUS_CODE: return "ok";
        case DATABASE_NOT_FOUND_STATUS_CODE: return "not found";
        case DATABASE_CONFLICT_STATUS_CODE: return "conflicting key";
        case DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE: return "database file corrupted";
        case DATABASE_WODO_FOLDER_NOT_FOUND: return "not a wodo repository (or any of the parent directories): .wodo";
        case DATABASE_ERRNO: return "errno";
        default: return "unknown";
    }
}

database_status_code_t load_database_v1(FILE *file) {
    uint64_t length;

    if (fread(&length, sizeof(uint64_t), 1, file) != 1) {
        return DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE;
    };

    Database_File *db_file;

    for (uint64_t i = 0; i < length; ++i) {
        db_file = malloc(sizeof(Database_File));

        db_file->name = NULL;
        db_file->relative_filepath = NULL;
        db_file->view_absolute_filepath = NULL;
        db_file->view_deleted = false;

        uint64_t name_size;

        if (fread(&name_size, sizeof(uint64_t), 1, file) != 1) {
            goto corrupted_db_error;
        }

        if (name_size == 0) {
            goto corrupted_db_error;
        }

        db_file->name = malloc(name_size);

        if (fread(db_file->name, sizeof(char), name_size, file) != name_size) {
            goto corrupted_db_error;
        }

        uint64_t path_size;

        if (fread(&path_size, sizeof(uint64_t), 1, file) != 1) {
            goto corrupted_db_error;
        }

        if (path_size == 0) {
            goto corrupted_db_error;
        }

        db_file->view_absolute_filepath = malloc(path_size);

        if (fread(db_file->view_absolute_filepath, sizeof(char), path_size, file) != path_size) {
            goto corrupted_db_error;
        }

        uint8_t remind = 0;

        if (fread(&remind, sizeof(uint8_t), 1, file) != 1) {
            goto corrupted_db_error;
        }

        size_t base_filepath_size = strlen(wodo_current_working_directory) + 1;
        size_t relative_filepath_size = strlen(db_file->view_absolute_filepath) - base_filepath_size;

        db_file->relative_filepath = malloc(sizeof(char) * (relative_filepath_size + 1));

        if (db_file->relative_filepath == NULL) {
            fprintf(stderr, "fatal: could not allocate memory (%ld bytes) to relative file path: %s\n", relative_filepath_size + 1, strerror(errno));
            exit(1);
        }

        memcpy(db_file->relative_filepath, db_file->view_absolute_filepath + base_filepath_size, relative_filepath_size);
        db_file->relative_filepath[relative_filepath_size] = '\0';


        cl_arr_push(global_database.files, db_file);
    }

    fclose(file);

    return DATABASE_OK_STATUS_CODE;
corrupted_db_error:
    fclose(file);
    free_db_file(db_file);
    cl_arr_free(global_database.files);
    return DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE;
}

database_status_code_t load_database_v2(FILE *file) {
    uint64_t length;

    if (fread(&length, sizeof(uint64_t), 1, file) != 1) {
        return DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE;
    };

    Database_File *db_file;

    for (uint64_t i = 0; i < length; ++i) {
        db_file = malloc(sizeof(Database_File));

        db_file->name = NULL;
        db_file->relative_filepath = NULL;
        db_file->view_absolute_filepath = NULL;
        db_file->view_deleted = false;

        uint64_t name_size;

        if (fread(&name_size, sizeof(uint64_t), 1, file) != 1) {
            goto corrupted_db_error;
        }

        if (name_size == 0) {
            goto corrupted_db_error;
        }

        db_file->name = malloc(name_size);

        if (fread(db_file->name, sizeof(char), name_size, file) != name_size) {
            goto corrupted_db_error;
        }

        uint64_t path_size;

        if (fread(&path_size, sizeof(uint64_t), 1, file) != 1) {
            goto corrupted_db_error;
        }

        if (path_size == 0) {
            goto corrupted_db_error;
        }

        db_file->relative_filepath = malloc(path_size);

        if (fread(db_file->relative_filepath, sizeof(char), path_size, file) != path_size) {
            goto corrupted_db_error;
        }

        db_file->view_absolute_filepath = join_paths("%s/%s", wodo_current_working_directory, db_file->relative_filepath);

        cl_arr_push(global_database.files, db_file);
    }

    fclose(file);

    return DATABASE_OK_STATUS_CODE;
corrupted_db_error:
    fclose(file);
    free_db_file(db_file);
    cl_arr_free(global_database.files);
    return DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE;
}

database_status_code_t read_database(FILE *file) {
    if (fread(&global_database.version, sizeof(short), 1, file) != 1) {
        return DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE;
    }

    // this version is only possible without the magic bytes
    if (global_database.version == DBV_0) {
        return DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE;
    }

    switch (global_database.version) {
        case DBV_1: return load_database_v1(file);
        case DBV_2: return load_database_v2(file);
        default: break;
    }

    // invalid file version
    return DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE;
}

database_status_code_t database_load() {
    FILE *file = fopen(wodo_current_working_directory_db, "rb");

    if (file == NULL) {
        fprintf(stderr, "could not open database file: %s\n", strerror(errno));
        exit(1);
    }

    char magic_bytes[6] = {0}; // .WODO\0

    if (fread(&magic_bytes, sizeof(char), 6, file) != 6) {
        return DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE;
    }

    // load new database format
    if (strncmp(magic_bytes, db_file_magic_bytes, 5) == 0) {
        return read_database(file);
    }

    if (fseek(file, 0L, SEEK_SET) != 0) {
        fclose(file);

        return DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE;
    }

    // load old database format for retro-compatibility
    uint64_t length;
    
    if (fread(&length, sizeof(uint64_t), 1, file) != 1) {
        return DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE;
    };

    for (uint64_t i = 0; i < length; ++i) {
        Database_File *db_file = malloc(sizeof(Database_File));

        db_file->name = NULL;
        db_file->view_absolute_filepath = NULL;
        db_file->relative_filepath = NULL;
        db_file->view_deleted = false;

        uint64_t name_size;

        if (fread(&name_size, sizeof(uint64_t), 1, file) != 1) {
            fclose(file);
            free_db_file(db_file);
            cl_arr_free(global_database.files);
            return DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE;
        }

        if (name_size == 0) {
            fclose(file);
            free_db_file(db_file);
            cl_arr_free(global_database.files);
            return DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE;
        }

        db_file->name = malloc(name_size);

        if (fread(db_file->name, sizeof(char), name_size, file) != name_size) {
            fclose(file);
            free_db_file(db_file);
            cl_arr_free(global_database.files);
            return DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE;
        }

        uint64_t path_size;

        if (fread(&path_size, sizeof(uint64_t), 1, file) != 1) {
            fclose(file);
            free_db_file(db_file);
            cl_arr_free(global_database.files);
            return DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE;
        }

        if (path_size == 0) {
            fclose(file);
            free_db_file(db_file);
            cl_arr_free(global_database.files);
            return DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE;
        }

        db_file->relative_filepath = malloc(path_size);

        if (fread(db_file->relative_filepath, sizeof(char), path_size, file) != path_size) {
            fclose(file);
            free_db_file(db_file);
            cl_arr_free(global_database.files);
            return DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE;
        }

        cl_arr_push(global_database.files, db_file);
    }

    fclose(file);

    return DATABASE_OK_STATUS_CODE;
}

char *database_init(const char *base_path) {
    wodo_current_working_directory = join_paths("%s/%s", base_path, db_folder_name);
    wodo_current_working_directory_db = join_paths("%s/%s", wodo_current_working_directory, db_filename);

    if (mkdir(wodo_current_working_directory, 0777) != 0) {
        fprintf(stderr, "error: could not init repository %s: %s\n", wodo_current_working_directory, strerror(errno));
        exit(1);
    }

    database_save();

    return wodo_current_working_directory;
}

void database_free() {
    if (wodo_current_working_directory != NULL) {
        free(wodo_current_working_directory);
    }
    if (wodo_current_working_directory_db != NULL) {
        free(wodo_current_working_directory_db);
    }

    for (size_t i = 0; i < cl_arr_len(global_database.files); i++) {
        Database_File *it = global_database.files[i];

        free_db_file(it);
    };

    cl_arr_free(global_database.files);
}

database_status_code_t database_get_file_by_filepath(Database_File **out, const char *filepath) {
    for (size_t i = 0; i < cl_arr_len(global_database.files); i++) {
        Database_File *it = global_database.files[i];

        if (it->view_deleted) continue;

        if (strncmp(filepath, it->view_absolute_filepath, strlen(filepath)) == 0) {
            if (out != NULL)
                *out = it;

            return DATABASE_OK_STATUS_CODE;
        }
    }

    return DATABASE_NOT_FOUND_STATUS_CODE;
}

database_status_code_t database_add_file(char *name, char **out_absolute_filepath) {
    Database_File_Path db_filepath = get_unix_filepath(name, strlen(name));

    for (size_t i = 0; i < cl_arr_len(global_database.files); i++) {
        Database_File *it = global_database.files[i];

        if (it->view_deleted) continue;

        if (strncmp(db_filepath.absolute, it->view_absolute_filepath, strlen(it->view_absolute_filepath)) == 0) {
            return DATABASE_CONFLICT_STATUS_CODE;
        }
    };

    FILE* fd = fopen(db_filepath.absolute, "w");

    if (fd == NULL) return DATABASE_ERRNO;

    fclose(fd);

    Database_File *file = malloc(sizeof(Database_File));

    file->view_absolute_filepath = db_filepath.absolute;
    file->relative_filepath = db_filepath.relative;
    file->view_deleted = false;
    file->name = name;

    cl_arr_push(global_database.files, file);

    if (out_absolute_filepath != NULL) {
        *out_absolute_filepath = file->view_absolute_filepath;
    }

    database_save();

    return DATABASE_OK_STATUS_CODE;
}

database_status_code_t database_delete_file(const char *absolute_filepath) {
    database_status_code_t status_code;

    Database_File *file = NULL;

    status_code = database_get_file_by_filepath(&file, absolute_filepath);

    if (status_code == DATABASE_NOT_FOUND_STATUS_CODE) {
        return 0;
    }

    if (status_code != DATABASE_OK_STATUS_CODE) {
        return status_code;
    }

    if (file == NULL) return 0;

    file->view_deleted = true;

    database_save();

    return DATABASE_OK_STATUS_CODE;
}

database_status_code_t database_rename_file(const char *absolute_filepath, const char *name) {
    for (size_t i = 0; i < cl_arr_len(global_database.files); i++) {
        Database_File *file = global_database.files[i];

        if (strncmp(file->view_absolute_filepath, absolute_filepath, strlen(file->view_absolute_filepath)) == 0) {
            size_t name_size = strlen(name);

            memcpy((void *)file->name, name, name_size);

            file->name[name_size] = '\0';

            database_save();

            return DATABASE_OK_STATUS_CODE;
        }
    }

    return DATABASE_NOT_FOUND_STATUS_CODE;
}
