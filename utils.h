#ifndef _WODO_UTILS_H_
#define _WODO_UTILS_H_

#include <stdbool.h>
#include <stddef.h>

const char *get_user_home_folder(void);
bool file_exists(const char *filepath, bool is_folder);
// the `size` is the size of the string in bytes, so it helps when it's not a null terminated string
size_t chars_count(const char *text, size_t size);

#endif // _WODO_UTILS_H_
