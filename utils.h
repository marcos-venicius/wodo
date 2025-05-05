#ifndef _WODO_UTILS_H_
#define _WODO_UTILS_H_

#include <stdbool.h>

const char *get_user_home_folder(void);
bool file_exists(const char *filepath, bool is_folder);

#endif // _WODO_UTILS_H_
