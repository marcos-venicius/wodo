#define CL_ARRAY_IMPLEMENTATION
#include <string.h>
#include <stdio.h>
#include "./json.h"
#include "./clibs/arr.h"
#include "./visualizer.h"
#include "./database.h"
#include "./utils.h"
#include "./io.h"
#include "./parser.h"

static bool task_match_flags(wodo_task_t task, Flags flags) {
    bool matched_any_states = cl_arr_len(flags.state_filter) == 0;

    wodo_task_state_t task_state = task.state_property.as.state_property;

    for (size_t i = 0; i < cl_arr_len(flags.state_filter); i++) {
        const char *state = flags.state_filter[i];

        size_t state_size = strlen(state);

        if (strncmp(state, "todo", state_size) == 0) {
            if (task_state == Wodo_Task_State_Todo) {
                matched_any_states = true;
                break;
            }
        } else if (strncmp(state, "doing", state_size) == 0) {
            if (task_state == Wodo_Task_State_Doing) {
                matched_any_states = true;
                break;
            }
        } else if (strncmp(state, "blocked", state_size) == 0) {
            if (task_state == Wodo_Task_State_Blocked) {
                matched_any_states = true;
                break;
            }
        } else if (strncmp(state, "done", state_size) == 0) {
            if (task_state == Wodo_Task_State_Done) {
                matched_any_states = true;
                break;
            }
        }

        break;
    }

    return matched_any_states;
}

void print_tasks_to_stdout_as_json(wodo_task_t *tasks, Flags flags) {
    int comma_index = 0;

    printf("[");
    for (size_t i = 0; i < cl_arr_len(tasks); i++) {
        wodo_task_t task = tasks[i];

        if (!task_match_flags(task, flags)) continue;

        // separate objects
        if (comma_index > 0) printf(",");
        comma_index++;

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
    printf("]");
}

void print_database_files_to_stdout_as_json(Flags flags) {
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

        bool matched_any_tasks = false;

        for (size_t i = 0; i < cl_arr_len(tasks); i++) {
            wodo_task_t task = tasks[i];

            if (!task_match_flags(task, flags)) continue;

            matched_any_tasks = true;

            switch (task.state_property.as.state_property) {
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
            print_tasks_to_stdout_as_json(tasks, flags);
        }
        printf("}");

        free(content);
        cl_arr_free(tasks);
    };
    printf("]\n");
}
