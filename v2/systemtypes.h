#ifndef _WODO_SYSTEMTYPES_H_
#define _WODO_SYSTEMTYPES_H_

#include <time.h>
#include <stddef.h>

typedef struct {
    int year, month, day, hour, minute, second;

    // Minutes
    int tz_offset;
} wodo_datetime_t;

typedef struct {
    const char *value;
    size_t     length;
} wodo_string_t;

typedef enum { 
    Wodo_Task_State_Todo,       // todo
    Wodo_Task_State_Doing,      // doing
    Wodo_Task_State_Blocked,    // blocked
    Wodo_Task_State_Done,       // done
} wodo_task_state_t;

typedef struct {
    wodo_string_t title;
    wodo_string_t description;

    wodo_task_state_t   state;
    wodo_string_t       *tags; // CL_ARRAY
    wodo_datetime_t     created_at;
} wodo_task_t;

#endif // !_WODO_SYSTEMTYPES_H_

