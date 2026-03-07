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
    int line, col;
} wodo_location_t;

typedef enum {
    wodo_node_kind_title,
    wodo_node_kind_description,
    wodo_node_kind_state_property,
    wodo_node_kind_tags_property,
    wodo_node_kind_date_property,
    wodo_node_kind_tag,
} wodo_node_kind_enum_t;

typedef struct wodo_node_t wodo_node_t;

struct wodo_node_t {
    wodo_location_t       location;
    wodo_node_kind_enum_t kind;
    union {
        wodo_string_t     title;
        wodo_string_t     description;
        wodo_task_state_t state_property;
        wodo_node_t       *tags_property; // CL_ARRAY
        wodo_datetime_t   date_property;
        wodo_string_t     tag;
    } as;
};

typedef struct {
    wodo_node_t title;
    wodo_node_t description;
    wodo_node_t state_property;
    wodo_node_t tags_property;
    wodo_node_t date_property;
} wodo_task_t;

#endif // !_WODO_SYSTEMTYPES_H_

