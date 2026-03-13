#ifndef _WODO_JSON_H_
#define _WODO_JSON_H_

#include <stdbool.h>
#include "./systemtypes.h"
#include "argparser.h"

void print_tasks_to_stdout_as_json(wodo_task_t *tasks, bool (*predicate)(wodo_task_t, Flags*), Flags *flags);
void print_database_files_to_stdout_as_json(bool (*predicate)(wodo_task_t, Flags*), Flags *flags);

#endif // !_WODO_JSON_H_
