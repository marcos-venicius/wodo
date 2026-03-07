#define CL_ARRAY_IMPLEMENTATION
#include <sys/ioctl.h>
#include <unistd.h>
#include <ctype.h>
#include <assert.h>
#include <stdio.h>
#include "./visualizer.h"
#include "./date.h"
#include "./clibs/arr.h"

static void print_wodo_datetime(wodo_datetime_t timezoned_datetime, bool simple)
{
    wodo_datetime_t datetime = convert_to_local(timezoned_datetime);

    if (simple) {
        printf("%04d-%02d-%02d %02d:%02d",
               datetime.year,
               datetime.month,
               datetime.day,
               datetime.hour,
               datetime.minute);
    } else {
        printf("%04d-%02d-%02d %02d:%02d:%02d",
               datetime.year,
               datetime.month,
               datetime.day,
               datetime.hour,
               datetime.minute,
               datetime.second);

        if (datetime.tz_offset == 0) {
            printf("+00:00");
        } else {
            int offset = datetime.tz_offset;
            char sign = '+';

            if (offset < 0) {
                sign = '-';
                offset = -offset;
            }

            int hours = offset / 60;
            int minutes = offset % 60;

            printf("%c%02d:%02d", sign, hours, minutes);
        }
    }
}

static void print_wodo_task_state(wodo_task_state_t state) {
    switch (state) {
        case Wodo_Task_State_Todo:
            printf("\033[0;33mTODO\033[0m   ");
            break;
        case Wodo_Task_State_Doing:
            printf("\033[0;34mDOING\033[0m  ");
            break;
        case Wodo_Task_State_Blocked:
            printf("\033[0;31mBLOCKED\033[0m");
            break;
        case Wodo_Task_State_Done:
            printf("\033[0;32mDONE\033[0m   ");
            break;
        default:
            assert(0 && "unimplemented task state visualizer");
    }
}

static void print_wodo_task_tags(wodo_string_t *tags) {
    printf("[");
    for (size_t i = 0; i < cl_arr_len(tags); i++) {
        if (i > 0) printf(", ");
        printf("%.*s", (int)tags[i].length, tags[i].value);
    }
    printf("]");
}

static int get_terminal_width(void)
{
    struct winsize w;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1 || w.ws_col == 0)
        return 120;  // fallback TODO: get from an env var

    return w.ws_col - 10;
}

static void print_wrapped_line(const char *line, size_t len)
{
    size_t pos = 0;
    size_t width = get_terminal_width();

    while (pos < len) {
        size_t remaining = len - pos;

        if (remaining <= width) {
            printf("%.*s\n", (int)remaining, line + pos);
            break;
        }

        size_t cut = width;

        while (cut > 0 && !isspace((unsigned char)line[pos + cut]))
            cut--;

        if (cut == 0)
            cut = width;

        printf("%.*s\n", (int)cut, line + pos);

        pos += cut;

        while (pos < len && isspace((unsigned char)line[pos]) && line[pos] != '\n')
            pos++;
    }
}

void print_wrapped_normalized(wodo_string_t s)
{
    size_t start = 0;
    size_t end = s.length;

    /* trim leading newlines */
    while (start < end && (s.value[start] == '\n' || s.value[start] == '\r'))
        start++;

    /* trim trailing newlines */
    while (end > start && (s.value[end-1] == '\n' || s.value[end-1] == '\r'))
        end--;

    if (start >= end)
        return;

    printf("\n");

    size_t pos = start;

    while (pos < end) {

        /* find end of current line */
        size_t line_start = pos;

        while (pos < end && s.value[pos] != '\n')
            pos++;

        size_t line_len = pos - line_start;

        if (line_len == 0) {
            printf("\n");
        } else {
            print_wrapped_line(s.value + line_start, line_len);
        }

        pos++; /* skip newline */
    }
}

void visualize_task(wodo_task_t task, bool full) {
    if (!full) {
        print_wodo_datetime(task.created_at, true);
        printf(" ");
    }

    switch (task.state) {
        case Wodo_Task_State_Todo:
            printf("\033[0;33m");
            break;
        case Wodo_Task_State_Doing:
            printf("\033[0;34m");
            break;
        case Wodo_Task_State_Blocked:
            printf("\033[0;31m");
            break;
        case Wodo_Task_State_Done:
            printf("\033[0;32m");
            break;
        default:
            assert(0 && "unimplemented task state visualizer");
    }
    printf("%% %.*s\033[0m", (int)task.title.length, task.title.value);
    if (full) {
        printf("\n\n");
        printf(".state ");
        print_wodo_task_state(task.state);
        printf("\n");
        if (cl_arr_len(task.tags) > 0) {
            printf(".tags  ");
            print_wodo_task_tags(task.tags);
            printf("\n");
        }
        printf(".date  ");
        print_wodo_datetime(task.created_at, false);
        printf("\n");
        print_wrapped_normalized(task.description);
    } else {
        if (cl_arr_len(task.tags) > 0) {
            printf(" ");
            print_wodo_task_tags(task.tags);
        }

        printf("\n");
    }
}
