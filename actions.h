#ifndef _WODO_ACTIONS_H_
#define _WODO_ACTIONS_H_

#include "argparser.h"
#include "database.h"

int add_path_action(const char *name, const char *filepath);
int add_action(const char *name);
int remove_action(const char *filepath);
int parse_as_json_action(const char *filepath, Flags flags);
int list_action(Flags flags);
int format_action(const char *filepath);
int rename_action(const char *filepath, char *title);
int get_reminders_action();

#endif // !_WODO_ACTIONS_H_
