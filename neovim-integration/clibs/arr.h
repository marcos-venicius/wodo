#ifndef CL_ARR_H_
#define CL_ARR_H_

#include <stddef.h>
#include <assert.h>

// headers
#define CL_ARRAY_INIT NULL
#define CL_ARRAY_INIT_CAPACITY 126

#define clean_errno() (errno == 0 ? "None" : strerror(errno))
#define log_error(M, ...) fprintf(stderr, "[ERROR] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)
#define assertf(A, M, ...) if(!(A)) {log_error(M, ##__VA_ARGS__); assert(A); }

static inline void *cl_arr_pop_impl(void *arr, size_t element_size);

typedef struct {
    size_t count;
    size_t capacity;
} CL_ArrayHeader;

// implementation

#ifdef CL_ARRAY_IMPLEMENTATION
#include <stdlib.h>

#define cl_arr_push(arr, v)                                                                 \
    do {                                                                                    \
        CL_ArrayHeader *header;                                                             \
        if ((arr) == CL_ARRAY_INIT) {                                                       \
            size_t amt = sizeof(*(arr)) * CL_ARRAY_INIT_CAPACITY + sizeof(CL_ArrayHeader);  \
            header = malloc(amt);                                                           \
            header->count = 0;                                                              \
            header->capacity = CL_ARRAY_INIT_CAPACITY;                                      \
            (arr) = (void*)(header + 1);                                                    \
        } else {                                                                            \
            header = (CL_ArrayHeader*)(arr) - 1;                                            \
        }                                                                                   \
                                                                                            \
        if (header->count >= header->capacity) {                                            \
            size_t new_cap = (header->capacity * 3) / 2 + 1;                                \
            size_t new_size = sizeof(*(arr)) * new_cap + sizeof(CL_ArrayHeader);            \
            CL_ArrayHeader *tmp = realloc(header, new_size);                                \
            if (tmp) {                                                                      \
                header = tmp;                                                               \
                header->capacity = new_cap;                                                 \
                (arr) = (void*)(header + 1);                                                \
            }                                                                               \
        }                                                                                   \
        (arr)[header->count++] = (v);                                                       \
    } while (0)

#define cl_arr_len(arr) ((arr) ? ((CL_ArrayHeader*)(arr) - 1)->count : 0)

#define cl_arr_free(arr)                    \
    do {                                    \
        if (arr != CL_ARRAY_INIT) {         \
            free((CL_ArrayHeader*)arr - 1); \
            arr = CL_ARRAY_INIT;            \
        }                                   \
    } while(0)

inline void *cl_arr_pop_impl(void *arr, size_t element_size) {
    assert((arr) != CL_ARRAY_INIT && "array was not initialized yet");

    CL_ArrayHeader *header = (CL_ArrayHeader*)(arr) - 1;
    assert(header->count > 0 && "you can't pop from an empty array");

    header->count--;

    return (char *)arr + header->count * element_size;
}

#define cl_arr_pop(arr) \
    ( \
        assert((arr) != CL_ARRAY_INIT && "array not initialized"), \
        assert((((CL_ArrayHeader *)(arr) -1)->count > 0) && "cannot pop from empty array"), \
        ((arr)[--((CL_ArrayHeader *)(arr) - 1)->count]) \
    )

#define cl_arr_u_remove(arr, i)                                         \
do {                                                                    \
    if ((arr) == CL_ARRAY_INIT) break;                                  \
    CL_ArrayHeader *header = (CL_ArrayHeader*)arr -1;                   \
    size_t index = (i);                                                 \
    assertf(index < header->count, "index %ld out of range", index);    \
    header->count--;                                                    \
    (arr)[index] = (arr)[header->count];                                \
} while(0)

// ADD ordered remove

#endif // CL_ARRAY_IMPLEMENTATION

#endif // CL_ARR_H_
