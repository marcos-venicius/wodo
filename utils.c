#include "./utils.h"
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
                string_size++;

                resulting_path = realloc(resulting_path, string_size);
                resulting_path[string_size - 1] = '/';
            } break;
            default: {
                free(resulting_path);
                return NULL;
            } 
        }
    }

    return resulting_path;
}

unsigned long get_current_timestamp(void) {
    return (unsigned long)time(NULL);
}
