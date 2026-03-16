#ifndef _WODO_CROSSPLATFORMOPS_H_
#define _WODO_CROSSPLATFORMOPS_H_

#ifndef FILENAME_MAX
#define FILENAME_MAX 1024
#endif

#ifdef _WIN32
    #include <direct.h>
    // Define an alias for consistency
    #define GetCurrentDir _getcwd
#else
    #include <unistd.h>
    // Define an alias for consistency
    #define GetCurrentDir getcwd
#endif

#endif // !_WODO_CROSSPLATFORMOPS_H_
