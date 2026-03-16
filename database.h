#ifndef _WODO_DATABASE_H_
#define _WODO_DATABASE_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define DATABASE_OK_STATUS_CODE 0
#define DATABASE_NOT_FOUND_STATUS_CODE 1
#define DATABASE_CONFLICT_STATUS_CODE 2
#define DATABASE_CORRUPTED_DATABASE_FILE_STATUS_CODE 3
#define DATABASE_WODO_FOLDER_NOT_FOUND 4
#define DATABASE_ERRNO 5

#define DBV_0 0 // old format (no version and no magic bytes)
#define DBV_1 1 // new format (with magic bytes and version)

typedef struct {
    char *name;
    char *filepath;
    bool remind; // if neovim needs to be reminded
    
    bool deleted; // not saved on database. Only to know if this was deleted or not before save
} Database_Db_File;

typedef struct {
    Database_Db_File **files;

    // DBV_0 | DBV_1. Default is DBV_0
    short version;
} Database;

typedef uint8_t database_status_code_t;

/*
 * Global database variable.
 * 
 * It's going to be the first thing that is loaded in the main function
 **/
extern Database global_database;

database_status_code_t load_wodo_database_working_directory();
// TODO: maybe in the future, do not load the entire database in memory
database_status_code_t database_load();
char *database_init(const char *base_path);
bool has_repository_at(const char *base_path);
void database_save();
void database_free();
database_status_code_t database_get_file_by_filepath(Database_Db_File **out, const char *filepath);
database_status_code_t database_add_file(Database_Db_File *file);
void database_delete_file(Database_Db_File *file);
void database_rename_file(Database_Db_File *file, char *name);
const char *database_status_code_string(database_status_code_t status_code);
// this function returns a file path inside the data folder with 
// a hash of the name suffixed with the current unix UTC timestamp
// /path/to/data/folder/<name-hash>-unix.wodo
// You need to free the memory of the return string when you're done
char *get_unix_filepath(const char *name, size_t name_size);

#endif // _WODO_DATABASE_H_
