#define CL_ARRAY_IMPLEMENTATION

#include <stdio.h>
#include "./io.h"
#include "./parser.h"
#include "./clibs/arr.h"
#include "./json.h"

int main() {
    char *content = NULL;

    const char *filename = "./examples/new-format.wodo";

    size_t length = read_from_file(filename, &content);

    wodo_task_t *tasks = parse_tasks(filename, content, length);

    print_tasks_to_stdout_as_json(tasks);

    cl_arr_free(tasks);

    return 0;
}
