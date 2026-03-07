#define CL_ARRAY_IMPLEMENTATION
#include "./database.h"
#include "./utils.h"
#include "./clibs/arr.h"
#include "./crypt.h"
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

static const char *db_folder_name = ".wodo";
static const char *file_extension = ".wodo";
static const char *db_filename = ".wodo.db";

static const char *get_database_filepath(void) {
    const char *user_home_folder = get_user_home_folder();

    char *database_filepath = malloc(strlen(user_home_folder) + strlen(db_folder_name) + strlen(db_filename) + 4);

    snprintf(database_filepath, strlen(user_home_folder) + strlen(db_folder_name) + 2, "%s/%s", user_home_folder, db_folder_name);

    if (!file_exists(database_filepath, true)) {
        if (mkdir(database_filepath, 0777) != 0) {
            fprintf(stderr, "error: could not created folder %s due to: %s\n", database_filepath, strerror(errno));
            exit(1);
        }
    }

    snprintf(database_filepath, strlen(user_home_folder) + strlen(db_folder_name) + strlen(db_filename) + 3, "%s/%s/%s", user_home_folder, db_folder_name, db_filename);

    return database_filepath;
}

static void free_db_file(Database_Db_File *file) {
    if (file->name != NULL) {
        free(file->name); 
        file->name = NULL;
    }

    if (file->filepath != NULL) {
        free(file->filepath);
        file->filepath = NULL;
    }

    free(file);
}

const char *database_status_code_string(database_status_code_t status_code) {
    switch (status_code) {
        case DATABASE_OK_STATUS_CODE: return "OK";
        case DATABASE_NOT_FOUND_STATUS_CODE: return "NOT FOUND";
        case DATABASE_CONFLICT_STATUS_CODE: return "CONFLICTING KEY";
        case DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE: return "DATABASE FILE CORRUPTED";
        default: return "UNKNOWN";
    }
}

database_status_code_t database_load(Database *out) {
    const char *database_filepath = get_database_filepath();

    if (!file_exists(database_filepath, false)) {
        return DATABASE_OK_STATUS_CODE;
    }

    FILE *file = fopen(database_filepath, "rb");

    if (file == NULL) {
        fprintf(stderr, "could not open database file: %s\n", strerror(errno));
        exit(1);
    }

    uint64_t length;
    
    if (fread(&length, sizeof(uint64_t), 1, file) != 1) {
        return DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE;
    };

    for (uint64_t i = 0; i < length; ++i) {
        Database_Db_File *db_file = malloc(sizeof(Database_Db_File));

        db_file->name = NULL;
        db_file->filepath = NULL;
        db_file->deleted = false;

        uint64_t name_size;

        if (fread(&name_size, sizeof(uint64_t), 1, file) != 1) {
            fclose(file);
            free_db_file(db_file);
            cl_arr_free(out->files);
            return DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE;
        }

        if (name_size == 0) {
            fclose(file);
            free_db_file(db_file);
            cl_arr_free(out->files);
            return DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE;
        }

        db_file->name = malloc(name_size);

        if (fread(db_file->name, sizeof(char), name_size, file) != name_size) {
            fclose(file);
            free_db_file(db_file);
            cl_arr_free(out->files);
            return DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE;
        }

        uint64_t path_size;

        if (fread(&path_size, sizeof(uint64_t), 1, file) != 1) {
            fclose(file);
            free_db_file(db_file);
            cl_arr_free(out->files);
            return DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE;
        }

        if (path_size == 0) {
            fclose(file);
            free_db_file(db_file);
            cl_arr_free(out->files);
            return DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE;
        }

        db_file->filepath = malloc(path_size);

        if (fread(db_file->filepath, sizeof(char), path_size, file) != path_size) {
            fclose(file);
            free_db_file(db_file);
            cl_arr_free(out->files);
            return DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE;
        }

        cl_arr_push(out->files, db_file);
    }

    fclose(file);

    return DATABASE_OK_STATUS_CODE;
}

void database_save(Database *database) {
    const char *database_filepath = get_database_filepath();

    FILE *file = fopen(database_filepath, "wb");

    if (file == NULL) {
        fprintf(stderr, "could not open database file: %s\n", strerror(errno));
        exit(1);
    }

    size_t length = 0;

    for (size_t i = 0; i < cl_arr_len(database->files); i++) {
        Database_Db_File *it = database->files[i];

        if (it->deleted) continue;

        length++;
    }

    fwrite(&length, sizeof(uint64_t), 1, file);

    for (size_t i = 0; i < cl_arr_len(database->files); i++) {
        Database_Db_File *it = database->files[i];

        if (it->deleted) continue;
        uint64_t name_size = strlen(it->name) + 1; // needs to save the null terminator
        uint64_t path_size = strlen(it->filepath) + 1; // needs to save the null terminator

        fwrite(&name_size, sizeof(uint64_t), 1, file);
        fwrite(it->name, sizeof(char), name_size, file);
        fwrite(&path_size, sizeof(uint64_t), 1, file);
        fwrite(it->filepath, sizeof(char), path_size, file);
    }

    fclose(file);
}

void database_free(Database *database) {
    for (size_t i = 0; i < cl_arr_len(database->files); i++) {
        Database_Db_File *it = database->files[i];

        free_db_file(it);
    };

    cl_arr_free(database->files);
}

database_status_code_t database_get_file_by_filepath(Database_Db_File **out, const Database *database, const char *filepath) {
    for (size_t i = 0; i < cl_arr_len(database->files); i++) {
        Database_Db_File *it = database->files[i];

        if (it->deleted) continue;

        if (strncmp(filepath, it->filepath, strlen(filepath)) == 0) {
            *out = it;

            return DATABASE_OK_STATUS_CODE;
        }
    }

    return DATABASE_NOT_FOUND_STATUS_CODE;
}

database_status_code_t database_add_file(Database *database, Database_Db_File *file) {
    for (size_t i = 0; i < cl_arr_len(database->files); i++) {
        Database_Db_File *it = database->files[i];
        if (it->deleted) continue;

        if (strncmp(file->filepath, it->filepath, strlen(it->filepath)) == 0) {
            return DATABASE_CONFLICT_STATUS_CODE;
        }
    };

    cl_arr_push(database->files, file);

    return DATABASE_OK_STATUS_CODE;
}

char *get_unix_filepath(const char *name, size_t name_size) {
    const char *user_home_folder = get_user_home_folder();
    unsigned char *hash = hash_bytes(name, name_size);
    unsigned long timestamp = get_current_timestamp();
    char *filepath = malloc(256);

    snprintf(filepath, 256, "%s/%s/%s-%lu%s", user_home_folder, db_folder_name, hash, timestamp, file_extension);

    free(hash);

    return filepath;
}

void database_delete_file(Database_Db_File *file)
{
    file->deleted = true;
}
