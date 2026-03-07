#ifndef _WODO_JSON_H_
#define _WODO_JSON_H_

#include "./systemtypes.h"
#include "./database.h"

void print_tasks_to_stdout_as_json(wodo_task_t *tasks);
void print_database_files_to_stdout_as_json(Database *database);

#endif // !_WODO_JSON_H_
