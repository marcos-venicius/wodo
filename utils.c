#define CL_ARRAY_IMPLEMENTATION

#include <pwd.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <locale.h>
#include <bits/types/mbstate_t.h>
#include <wchar.h>
#include <stdio.h>
#include <stdarg.h>
#include "./utils.h"
#include "./clibs/arr.h"
#include <stdbool.h>
#include "./systemtypes.h"
#include "./argparser.h"

const char *get_user_home_folder(void) {
    const char *home = getenv("HOME");

    if (home == NULL) return getpwuid(getuid())->pw_dir;

    return home;
}

bool file_exists(const char *filepath, bool is_folder) {
    struct stat st;

    if (stat(filepath, &st) != 0) {
        return false;
    }

    if (is_folder) return S_ISDIR(st.st_mode);

    return !S_ISDIR(st.st_mode);
}

size_t chars_count(const char *text, size_t size) {
    setlocale(LC_ALL, "");

    size_t chars_count = 0;
    size_t read_bytes = 0;

    mbstate_t state;
    memset(&state, 0, sizeof(mbstate_t));

    const char *ptr = text;

    for (size_t i = 0; i < size && *ptr != '\0'; ++i) {
        wchar_t wc;

        int result = mbrtowc(&wc, ptr, size - read_bytes, &state);

        if (result > 0) {
            chars_count++;
            ptr += result;
            read_bytes += result;
        } else {
            ptr++;
        }
    }

    return chars_count;
}

char *join_paths(const char *text, ...) {
    va_list args;
    va_start(args, text);

    size_t string_size = 0;
    char *resulting_path = NULL;

    for (size_t i = 0; i < strlen(text); ++i) {
        switch (text[i]) {
            case '%': {
                if (i >= strlen(text) || text[i + 1] != 's') {
                    free(resulting_path);

                    return NULL;
                }

                char *path = va_arg(args, char*);

                size_t size = strlen(path);

                resulting_path = realloc(resulting_path, string_size + size);

                memcpy(resulting_path + string_size, path, size);

                string_size += size;

                i++;
            } break;
            case ' ': {
                string_size++;

                resulting_path = realloc(resulting_path, string_size);
                resulting_path[string_size - 1] = ' ';
            } break;
            case '/': {
                if (resulting_path[string_size - 1] != '/') {
                    string_size++;

                    resulting_path = realloc(resulting_path, string_size);
                    resulting_path[string_size - 1] = '/';
                }
            } break;
            default: {
                string_size++;

                resulting_path = realloc(resulting_path, string_size);
                resulting_path[string_size - 1] = text[i];
            } break;
        }
    }

    resulting_path = realloc(resulting_path, string_size + 1);
    resulting_path[string_size] = '\0';

    return resulting_path;
}

unsigned long get_current_timestamp(void) {
    return (unsigned long)time(NULL);
}

SysDateTime get_today_date(void) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    // Extract day, month, and year
    int day = tm.tm_mday;    // Day of the month (1-31)
    int month = tm.tm_mon + 1; // Month (0-11), so add 1
    int year = tm.tm_year + 1900; // Year since 1900, so add 1900
    int hour = tm.tm_hour;
    int minute = tm.tm_min;

    return (SysDateTime){.day = day, .month = month, .year = year, .hour = hour, .minute = minute };
}

time_t get_timestamp(SysDateTime sys_date_time) {
    struct tm t = {0};

    t.tm_year = sys_date_time.year - 1900;
    t.tm_mon = sys_date_time.month - 1;
    t.tm_mday = sys_date_time.day;
    t.tm_hour = sys_date_time.hour;
    t.tm_min = sys_date_time.minute;
    t.tm_sec = 0;

    time_t timestamp = mktime(&t);

    if (timestamp == -1) {
        fprintf(stderr, "could not convert to timestamp\n");
        exit(1);
    }

    return timestamp;
}

bool cmp_sized_strings(const char *a, const char *b, size_t len_a, size_t len_b) {
    if (len_a != len_b) return false;

    for (size_t i = 0; i < len_a; ++i)
        if (a[i] != b[i]) return false;

    return true;
}

bool arg_cmp(const char *actual, const char *expected, const char *alternative) {
    size_t actual_length = strlen(actual);
    size_t expected_length = strlen(expected);
    size_t alternative_length = strlen(alternative);

    bool expected_result = cmp_sized_strings(actual, expected, actual_length, expected_length);
    bool alternative_result = cmp_sized_strings(actual, alternative, actual_length, alternative_length);

    return  expected_result || alternative_result;
}

bool arg_cmp_single(const char *actual, const char *expected) {
    size_t actual_length = strlen(actual);
    size_t expected_length = strlen(expected);

    bool expected_result = cmp_sized_strings(actual, expected, actual_length, expected_length);

    return  expected_result;
}

void print_scaped_string_to_fd(wodo_string_t string, FILE *file) {
    fprintf(file, "\"");
    size_t last_index = 0;

    for (size_t i = 0; i < string.length; i++) {
        switch (string.value[i]) {
            case '"': {
                fprintf(file, "%.*s", (int)(i - last_index), string.value + last_index);
                fprintf(file, "\\\"");

                // skip double quotes
                last_index = i + 1;
            } break;
            case '\n': {
                fprintf(file, "%.*s", (int)(i - last_index), string.value + last_index);
                fprintf(file, "\\n");

                // skip double quotes
                last_index = i + 1;
            } break;
        }
    }

    if (last_index < string.length)
        fprintf(file, "%.*s", (int)(string.length - last_index), string.value + last_index);

    fprintf(file, "\"");
}

bool default_task_predicate(wodo_task_t task, Flags flags) {
    bool matched_any_states = cl_arr_len(flags.state_filter) == 0;
    bool matched_any_tags = cl_arr_len(flags.tag_filter) == 0;

    if (matched_any_states && matched_any_tags) return true;

    wodo_task_state_t task_state = task.state_property.state;

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

    wodo_node_t *tags = task.tags_property.node_array;

    for (size_t i = 0; i < cl_arr_len(flags.tag_filter); i++) {
        const char *tag_filter = flags.tag_filter[i];

        for (size_t j = 0; j < cl_arr_len(tags); j++) {
            wodo_string_t task_tag = tags[j].string;

            if (strncmp(tag_filter, task_tag.value, task_tag.length) == 0) {
                matched_any_tags = true;

                break;
            }
        }

        if (matched_any_tags) break;
    }

    return matched_any_states && matched_any_tags;
}
