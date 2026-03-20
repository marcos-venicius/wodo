#include "io.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

size_t read_from_file(const char *filename, char **content) {
    FILE *fptr = fopen(filename, "r");

    if (fptr == NULL) {
        fprintf(stderr, "could not open file %s due to: %s\n", filename, strerror(errno));
        exit(1);
    }

    fseek(fptr, 0, SEEK_END);
    const size_t stream_size = ftell(fptr);
    rewind(fptr);

    if (content != NULL) {
        *content = malloc((stream_size + 1) * sizeof(char));

        if (*content == NULL) {
            fprintf(stderr, "could not open allocate memory enough: %s\n", strerror(errno));
            exit(1);
        }

        const size_t read_size = fread(*content, 1, stream_size, fptr);

        if (read_size != stream_size) {
            fprintf(stderr, "could not read file %s due to: %s\n", filename, strerror(errno));
            fclose(fptr);
            exit(1);
        }

        (*content)[stream_size] = '\0';
    } else {
        fprintf(stderr, "could not write file content because 'content' variable is null\n");
    }

    fclose(fptr);

    return stream_size;
}

size_t read_from_stdin(char **content) {
    char *line = NULL;
    size_t len;
    size_t read_size = 0;
    size_t content_length = 0;
    char *buffer = NULL;

    while ((read_size = getline(&line, &len, stdin)) != -1) {
        size_t previous_length = content_length;
        content_length += read_size;

        // TODO: handle errors correctly
        buffer = realloc(buffer, content_length);

        memcpy(buffer + previous_length, line, read_size);
    }

    if (content != NULL) *content = buffer;

    return content_length;
}
