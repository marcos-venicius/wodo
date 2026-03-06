#define CL_ARRAY_IMPLEMENTATION

#include <stdio.h>
#include "./io.h"
#include "./parser.h"
#include "./clibs/arr.h"

void print_wodo_datetime(const wodo_datetime_t *dt)
{
    printf("%04d-%02d-%02d %02d:%02d:%02d",
           dt->year,
           dt->month,
           dt->day,
           dt->hour,
           dt->minute,
           dt->second);

    if (dt->tz_offset == 0) {
        printf("Z");
    } else {
        int offset = dt->tz_offset;
        char sign = '+';

        if (offset < 0) {
            sign = '-';
            offset = -offset;
        }

        int hours = offset / 60;
        int minutes = offset % 60;

        printf("%c%02d:%02d", sign, hours, minutes);
    }

    printf("\n");
}

int main() {
    char *content = NULL;

    const char *filename = "./examples/new-format.wodo";

    size_t length = read_from_file(filename, &content);

    wodo_task_t *tasks = parse_tasks(filename, content, length);

    for (size_t i = 0; i < cl_arr_len(tasks); ++i) {
        wodo_task_t task = tasks[i];
        printf("Title: %.*s\n", (int)task.title.length, task.title.value);
        printf("Date: ");
        print_wodo_datetime(&task.created_at);

        printf("State: ");

        switch (task.state) {
            case Wodo_Task_State_Todo:
                printf("todo\n");
                break;
            case Wodo_Task_State_Doing:
                printf("doing\n");
                break;
            case Wodo_Task_State_Blocked:
                printf("blocked\n");
                break;
            case Wodo_Task_State_Done:
                printf("done\n");
                break;
        }

        printf("Tags: [");
        for (size_t i = 0; i < cl_arr_len(task.tags); ++i) {
            if (i > 0) printf(", ");
            printf("%.*s", (int)task.tags[i].length, task.tags[i].value);
        }
        printf("]\n");

        printf("Description: %.*s\n", (int)task.description.length, task.description.value);
    }

    return 0;
}
