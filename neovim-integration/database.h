#ifndef _WODO_DATABASE_H_
#define _WODO_DATABASE_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define DATABASE_OK_STATUS_CODE 0
#define DATABASE_NOT_FOUND_STATUS_CODE 1
#define DATABASE_CONFLICT_STATUS_CODE 2
#define DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE 3

typedef struct {
    char *name;
    char *filepath;
    
    bool deleted; // not saved on database. Only to know if this was deleted or not before save
} Database_Db_File;

typedef struct {
    Database_Db_File **files;
} Database;

typedef uint8_t database_status_code_t;

/*
Global database variable.

It's going to be the first thing that is loaded in the main function
**/
extern Database global_database;

// TODO: maybe in the future, do not load the entire database in memory
database_status_code_t database_load(Database *out);
void database_save(Database *database);
void database_free(Database *database);
database_status_code_t database_get_file_by_filepath(Database_Db_File **out, const Database *database, const char *filepath);
database_status_code_t database_add_file(Database *database, Database_Db_File *file);
void database_delete_file(Database_Db_File *file);
const char *database_status_code_string(database_status_code_t status_code);
// this function returns a file path inside the data folder with 
// a hash of the name suffixed with the current unix UTC timestamp
// /path/to/data/folder/<name-hash>-unix.wodo
// You need to free the memory of the return string when you're done
char *get_unix_filepath(const char *name, size_t name_size);

#endif // _WODO_DATABASE_H_
