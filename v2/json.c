#define CL_ARRAY_IMPLEMENTATION
#include <string.h>
#include <stdio.h>
#include "./json.h"
#include "./clibs/arr.h"
#include "./visualizer.h"
#include "./database.h"
#include "./utils.h"

void print_tasks_to_stdout_as_json(wodo_task_t *tasks) {
    printf("[");
    for (size_t i = 0; i < cl_arr_len(tasks); i++) {
        // separate objects
        if (i > 0) printf(",");

        wodo_task_t task = tasks[i];

        printf("{");
        // title
        {
            printf("\"title\":{");
            printf("\"content\":");
            print_scaped_string_to_fd(task.title.as.title, stdout);
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
            switch (task.state_property.as.state_property) {
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
            print_wodo_datetime(task.date_property.as.date_property, false);
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
            wodo_node_t *tags = task.tags_property.as.tags_property;

            printf("\"tags\":{");
            // content
            {
                printf("\"content\":[");

                for (size_t i = 0; i < cl_arr_len(tags); i++) {
                    if (i > 0) printf(",");

                    wodo_node_t tag = tags[i];

                    printf("{");
                    printf("\"content\":\"%.*s\",", (int)tag.as.tag.length, tag.as.tag.value);
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

        // description
        {
            printf("\"description\":{");

            printf("\"content\":");

            print_scaped_string_to_fd(task.description.as.description, stdout);

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
    printf("]\n");
}

void print_database_files_to_stdout_as_json(Database *database) {
    printf("[");
    for (size_t i = 0; i < cl_arr_len(database->files); i++) {
        if (i > 0) printf(",");

        Database_Db_File *it = database->files[i];

        printf("{");
        printf("\"name\":");
        print_scaped_string_to_fd((wodo_string_t){
            .length = strlen(it->name),
            .value = it->name
        }, stdout);
        printf(",");
        printf("\"path\":\"%s\"", it->filepath);
        printf("}");
    };
    printf("]\n");
}

