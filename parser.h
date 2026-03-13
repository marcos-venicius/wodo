#ifndef _WODO_PARSER_H_
#define _WODO_PARSER_H_
#include <stddef.h>
#include "./systemtypes.h"

wodo_task_t *parse_tasks(const char *filename, const char *content, size_t length);
void reset_parser_state(void);

#endif // !_WODO_PARSER_H_
