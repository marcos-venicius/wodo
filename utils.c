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
