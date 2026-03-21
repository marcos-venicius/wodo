#ifndef _WODO_ACTIONS_H_
#define _WODO_ACTIONS_H_

#include "argparser.h"
#include "database.h"

int add_wodo_file_action(char *name);
int remove_wodo_file_action(const char *filepath);
// filepath here is gonna be used just for error reporting at
// precise locations. The content will be read from stdin.
int parse_wodo_file_from_stdin_action(const char *filepath, Flags flags);
int list_action(Flags flags);
// filepath here is gonna be used just for error reporting at
// precise locations. The content will be read from stdin.
int format_wodo_file_from_stdin_action(const char *filepath);
int rename_wodo_file_action(const char *filepath, char *title);
int get_reminders_action();
int init_repository_action();

#endif // !_WODO_ACTIONS_H_
