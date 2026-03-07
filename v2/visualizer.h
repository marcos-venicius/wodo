#ifndef _WODO_VISUALIZER_H_
#define _WODO_VISUALIZER_H_

#include <stdbool.h>
#include "./systemtypes.h"

void visualize_task(wodo_task_t task, bool full);
void print_wodo_datetime(wodo_datetime_t timezoned_datetime, bool simple);

#endif // !_WODO_VISUALIZER_H_
