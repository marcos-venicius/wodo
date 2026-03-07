#define CL_ARRAY_IMPLEMENTATION

#include <stdio.h>
#include "./io.h"
#include "./parser.h"
#include "./clibs/arr.h"
#include "./visualizer.h"


int main() {
    char *content = NULL;

    const char *filename = "./examples/new-format.wodo";

    size_t length = read_from_file(filename, &content);

    wodo_task_t *tasks = parse_tasks(filename, content, length);

    for (size_t i = 0; i < cl_arr_len(tasks); ++i) {
        if (i > 0) printf("\n");
        wodo_task_t task = tasks[i];

        visualize_task(task, false);
    }

    return 0;
}
