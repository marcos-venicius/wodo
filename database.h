#ifndef _WODO_DATABASE_H_
#define _WODO_DATABASE_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define database_foreach_begin(database) { uint64_t i = 0; while (i < (database).length) { Database_Db_File *it = (database).data[i++];
#define database_foreach_end }}

#define LARGE_IDENTIFIER_SIZE 40
#define SHORT_IDENTIFIER_SIZE 7

#define DATABASE_OK_STATUS_CODE 0
#define DATABASE_NOT_FOUND_STATUS_CODE 1
#define DATABASE_CONFLICT_STATUS_CODE 2
#define DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE 3

typedef struct {
    char *name;
    char *filepath;
    unsigned char large_identifier[LARGE_IDENTIFIER_SIZE]; // sha1?
    unsigned char short_identifier[SHORT_IDENTIFIER_SIZE]; // sha1[..SHORT_IDENTIFIER_SIZE]?
    
    bool deleted; // not saved on database. Only to know if this was deleted or not before save
} Database_Db_File;

typedef struct {
    uint64_t length;
    uint64_t capacity;

    Database_Db_File **data;
} Database;

typedef uint8_t database_status_code_t;

// TODO: maybe in the future, do not load the entire database in memory
database_status_code_t database_load(Database *out);
void database_save(Database *database);
void database_free(Database *database);
database_status_code_t database_get_file_by_identifier(Database_Db_File **out, const Database *database, const char identifier[40]);
database_status_code_t database_add_file(Database *database, Database_Db_File *file);
void database_delete_file(Database_Db_File *file);
const char *database_status_code_string(database_status_code_t status_code);

#endif // _WODO_DATABASE_H_
