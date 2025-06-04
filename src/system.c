#include "minigl/system.h"

#include <stdio.h>

void* minigl_fopen(char* path, char* flags) {
    return fopen(path, flags);
}

int minigl_fseek(void* f, int pos, int whence) {
    return fseek(f, pos, whence);
}

long minigl_ftell(void* f) {
    return ftell(f);
}

int minigl_fread(void* ptr, size_t size, void* stream) {
    return fread(ptr, size, 1, stream);
}

int minigl_fwrite(void* f, const void* buf, unsigned int len) {
    return fwrite(buf, 1, len, f);
}

int minigl_fclose(void* f) {
    return fclose(f);
}
