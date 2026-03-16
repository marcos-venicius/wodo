#define CL_ARRAY_IMPLEMENTATION
#include <string.h>
#include <stdio.h>
#include "json.h"
#include "arr.h"
#include "visualizer.h"
#include "database.h"
#include "utils.h"
#include "io.h"
#include "parser.h"

void print_tasks_to_stdout_as_json(wodo_task_t *tasks, bool (*predicate)(wodo_task_t, Flags), Flags flags) {
    int comma_index = 0;

    printf("[");
    for (size_t i = 0; i < cl_arr_len(tasks); i++) {
        wodo_task_t task = tasks[i];

        if (!predicate(task, flags)) continue;

        // separate objects
        if (comma_index > 0) printf(",");
        comma_index++;

        printf("{");
        // title
        {
            printf("\"title\":{");
            printf("\"content\":");
            print_scaped_string_to_fd(task.title.string, stdout);
            printf(",");
            // location
            {
                printf("\"location\":{");
                printf("\"line\":%d,", task.title.location.line);
                printf("\"col\":%d", task.title.location.col);
                printf("}");
            }
            printf("}");
        }

        printf(",");

        // state
        {
            printf("\"state\":{");
            printf("\"content\":");
            switch (task.state_property.state) {
                case Wodo_Task_State_Todo: printf("\"todo\""); break;
                case Wodo_Task_State_Doing: printf("\"doing\""); break;
                case Wodo_Task_State_Blocked: printf("\"blocked\""); break;
                case Wodo_Task_State_Done: printf("\"done\""); break;
                default: assert(0 && "unimplemented json parsing state");
            }
            // location
            if (task.state_property.location.col != 0) {
                printf(",");
                printf("\"location\":{");
                printf("\"line\":%d,", task.state_property.location.line);
                printf("\"col\":%d", task.state_property.location.col);
                printf("}");
            }

            printf("}");
        }

        printf(",");

        // date
        {
            printf("\"date\":{");
            printf("\"content\":\"");
            print_wodo_datetime(task.date_property.datetime, false);
            printf("\",");

            // location
            {
                printf("\"location\":{");
                printf("\"line\":%d,", task.date_property.location.line);
                printf("\"col\":%d", task.date_property.location.col);
                printf("}");
            }

            printf("}");
        }

        printf(",");

        // tags
        {
            wodo_node_t *tags = task.tags_property.node_array;

            printf("\"tags\":{");
            // content
            {
                printf("\"content\":[");

                for (size_t i = 0; i < cl_arr_len(tags); i++) {
                    if (i > 0) printf(",");

                    wodo_node_t tag = tags[i];

                    printf("{");
                    printf("\"content\":\"%.*s\",", (int)tag.string.length, tag.string.value);
                    // location
                    {
                        printf("\"location\":{");
                        printf("\"line\":%d,", tag.location.line);
                        printf("\"col\":%d", tag.location.col);
                        printf("}");
                    }
                    printf("}");
                }
                printf("]");
            }

            // location
            if (task.tags_property.location.col != 0) {
                printf(",");
                printf("\"location\":{");
                printf("\"line\":%d,", task.tags_property.location.line);
                printf("\"col\":%d", task.tags_property.location.col);
                printf("}");
            }

            printf("}");
        }

        printf(",");

        // remind
        {
            printf("\"remind\":{");
            printf("\"content\":%s,", task.remind_property.boolean ? "true" : "false");

            // location
            {
                printf("\"location\":{");
                printf("\"line\":%d,", task.date_property.location.line);
                printf("\"col\":%d", task.date_property.location.col);
                printf("}");
            }

            printf("}");
        }

        printf(",");

        // description
        {
            printf("\"description\":{");

            printf("\"content\":");

            print_scaped_string_to_fd(task.description.string, stdout);

            // location
            if (task.description.location.col != 0) {
                printf(",");
                printf("\"location\":{");
                printf("\"line\":%d,", task.description.location.line);
                printf("\"col\":%d", task.description.location.col);
                printf("}");
            }

            printf("}");
        }

        printf("}");
    }
    printf("]");
}

void print_database_files_to_stdout_as_json(bool (*predicate)(wodo_task_t task, Flags), Flags flags) {
    int total_count = 0;
    int todo_count = 0;
    int doing_count = 0;
    int blocked_count = 0;
    int done_count = 0;
    int comma_index = 0;

    printf("[");
    for (size_t i = 0; i < cl_arr_len(global_database.files); i++) {
        Database_Db_File *it = global_database.files[i];

        char *content;

        size_t length = read_from_file(it->filepath, &content);

        reset_parser_state();

        wodo_task_t *tasks = parse_tasks(it->filepath, content, length);

        bool matched_any_tasks = cl_arr_len(tasks) == 0;

        for (size_t i = 0; i < cl_arr_len(tasks); i++) {
            wodo_task_t task = tasks[i];

            if (!predicate(task, flags)) continue;

            matched_any_tasks = true;

            switch (task.state_property.state) {
                case Wodo_Task_State_Todo:
                    todo_count++;
                    break;
                case Wodo_Task_State_Doing:
                    doing_count++;
                    break;
                case Wodo_Task_State_Blocked:
                    blocked_count++;
                    break;
                case Wodo_Task_State_Done:
                    done_count++;
                    break;
                default: assert(0 && "unhandled wodo state during files listing");
            }
            total_count++;
        }

        if (!matched_any_tasks) {
            free(content);
            cl_arr_free(tasks);

            continue;
        }

        if (comma_index > 0) printf(",");

        comma_index++;

        printf("{");
        printf("\"name\":");
        print_scaped_string_to_fd((wodo_string_t){
            .length = strlen(it->name),
            .value = it->name
        }, stdout);
        printf(",");
        printf("\"path\":\"%s\"", it->filepath);
        printf(",");
        {
            printf("\"states\": {");
            printf("\"total\":%d,", total_count);
            printf("\"todo\":%d,", todo_count);
            printf("\"doing\":%d,", doing_count);
            printf("\"blocked\":%d,", blocked_count);
            printf("\"done\":%d", done_count);
            printf("}");
        }
        printf(",");
        {
            printf("\"tasks\":");
            print_tasks_to_stdout_as_json(tasks, predicate, flags);
        }
        printf("}");

        free(content);
        cl_arr_free(tasks);
    };
    printf("]\n");
}
