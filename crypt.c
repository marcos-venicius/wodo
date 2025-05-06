#include "./crypt.h"
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>

unsigned char *hash_bytes(const char *bytes, size_t size) {
    unsigned char tmp[SHA_DIGEST_LENGTH] = {0};

    SHA1((unsigned char*)bytes, size, tmp);

    unsigned char *hash = malloc(SHA_DIGEST_LENGTH * 2 + 1);

    for (int i = 0; i < SHA_DIGEST_LENGTH; ++i)
        sprintf((char*)(hash + i * 2), "%02x", tmp[i]);

    return hash;
}
