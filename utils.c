#include "./utils.h"
#include <pwd.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

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
