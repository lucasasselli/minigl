#pragma once

#include <stdio.h>

#define MINIGL_WEAK __attribute__((weak))

MINIGL_WEAK void* minigl_fopen(char* path, char* flags);

MINIGL_WEAK int minigl_fseek(void* f, int pos, int whence);

MINIGL_WEAK long minigl_ftell(void* f);

MINIGL_WEAK int minigl_fread(void* ptr, size_t size, void* f);

MINIGL_WEAK int minigl_fwrite(void* f, const void* buf, unsigned int len);

MINIGL_WEAK int minigl_fclose(void* f);
