#include "./database.h"
#include "./utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#define DEFAULT_ARRAY_CAPACITY 50

static const char *db_folder_name = ".wodo";
static const char *db_filename = ".wodo.db";

#define array_append(array, item) do { \
    if ((array)->length >= (array)->capacity) { \
        if ((array)->capacity == 0) (array)->capacity = DEFAULT_ARRAY_CAPACITY; \
        else (array)->capacity = (array)->capacity * 2; \
        void *output = realloc((array)->data, (array)->capacity * sizeof((array)->data[0])); \
        assert(output != NULL && "failed to reallocate array"); \
        (array)->data = output; \
    } \
    (array)->data[(array)->length++] = item; \
} while (0);

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

database_status_code_t database_load(Database **out) {
    const char *database_filepath = get_database_filepath();

    *out = malloc(sizeof(Database));

    (*out)->capacity = 0;
    (*out)->length = 0;
    (*out)->data = NULL;

    if (!file_exists(database_filepath, false)) {
        return DATABASE_OK_STATUS_CODE;
    }

    FILE *file = fopen(database_filepath, "rb");

    if (file == NULL) {
        fprintf(stderr, "could not open database file: %s\n", strerror(errno));
        exit(1);
    }

    if (fread(&((*out)->length), sizeof(uint64_t), 1, file) != 1) {
        return DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE;
    };

    (*out)->capacity = (*out)->length;
    (*out)->data = malloc(sizeof(Database_Db_File*) * (*out)->length);

    for (uint64_t i = 0; i < (*out)->length; ++i) {
        Database_Db_File *db_file = malloc(sizeof(Database_Db_File));

        db_file->name = NULL;
        db_file->filepath = NULL;
        db_file->deleted = false;

        uint64_t name_size;

        if (fread(&name_size, sizeof(uint64_t), 1, file) != 1) {
            fclose(file);
            free_db_file(db_file);
            free(*out);
            return DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE;
        }

        if (name_size == 0) {
            fclose(file);
            free_db_file(db_file);
            free(*out);
            return DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE;
        }

        db_file->name = malloc(name_size);

        if (fread(db_file->name, sizeof(char), name_size, file) != name_size) {
            fclose(file);
            free_db_file(db_file);
            free(*out);
            return DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE;
        }

        uint64_t filepath_size;

        if (fread(&filepath_size, sizeof(uint64_t), 1, file) != 1) {
            fclose(file);
            free_db_file(db_file);
            free(*out);
            return DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE;
        }

        if (filepath_size == 0) {
            fclose(file);
            free_db_file(db_file);
            free(*out);
            return DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE;
        }

        db_file->filepath = malloc(filepath_size);

        if (fread(db_file->filepath, sizeof(char), filepath_size, file) != filepath_size) {
            fclose(file);
            free_db_file(db_file);
            free(*out);
            return DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE;
        }

        if (fread(db_file->large_identifier, sizeof(char), LARGE_IDENTIFIER_SIZE, file) != LARGE_IDENTIFIER_SIZE) {
            fclose(file);
            free_db_file(db_file);
            free(*out);
            return DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE;
        }

        if (fread(db_file->short_identifier, sizeof(char), SHORT_IDENTIFIER_SIZE, file) != SHORT_IDENTIFIER_SIZE) {
            fclose(file);
            free_db_file(db_file);
            free(*out);
            return DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE;
        }

        (*out)->data[i] = db_file;
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

    uint64_t actual_db_size = 0;

    database_foreach_begin(*database) {
        if (it->deleted) continue;
        actual_db_size++;
    } database_foreach_end;

    fwrite(&actual_db_size, sizeof(uint64_t), 1, file);

    database_foreach_begin(*database) {
        if (it->deleted) continue;

        uint64_t filename_size = strlen(it->name);
        uint64_t filepath_size = strlen(it->filepath);

        fwrite(&filename_size, sizeof(uint64_t), 1, file);
        fwrite(it->name, sizeof(char), filename_size, file);
        fwrite(&filepath_size, sizeof(uint64_t), 1, file);
        fwrite(it->filepath, sizeof(char), filepath_size, file);
        fwrite(it->large_identifier, sizeof(char), LARGE_IDENTIFIER_SIZE, file);
        fwrite(it->short_identifier, sizeof(char), SHORT_IDENTIFIER_SIZE, file);
    } database_foreach_end;

    fclose(file);
}

void database_free(Database *database) {
    database_foreach_begin(*database) {
        free_db_file(it);
    } database_foreach_end;

    database->length = 0;
    database->capacity = 0;
    free(database->data);
}

database_status_code_t database_get_file_by_identifier(Database_Db_File **out, const Database *database, const char identifier[40]) {
    database_foreach_begin(*database) {
        if (it->deleted) continue;

        if (strncmp(identifier, (char*)it->short_identifier, SHORT_IDENTIFIER_SIZE) == 0) {
            *out = it;

            return DATABASE_OK_STATUS_CODE;
        }
    } database_foreach_end;

    return DATABASE_NOT_FOUND_STATUS_CODE;
}

database_status_code_t database_add_file(Database *database, Database_Db_File *file) {
    database_foreach_begin(*database) {
        if (it->deleted) continue;

        if (strncmp((char*)file->short_identifier, (char*)it->short_identifier, SHORT_IDENTIFIER_SIZE) == 0) {
            return DATABASE_CONFLICT_STATUS_CODE;
        }
    } database_foreach_end;

    array_append(database, file);

    return DATABASE_OK_STATUS_CODE;
}

void database_delete_file(Database_Db_File *file)
{
    file->deleted = true;
}
