#ifndef WINTAKOS_STRING_H
#define WINTAKOS_STRING_H

#include "../include/types.h"

void* memset(void* dest, int val, size_t count);
void* memcpy(void* dest, const void* src, size_t count);
int   memcmp(const void* a, const void* b, size_t count);
size_t strlen(const char* str);

#endif
