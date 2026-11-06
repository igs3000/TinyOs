#ifndef STRING_H
#define STRING_H

#include "types.h"

size_t strlen(const char* s);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
char* strcat(char* dest, const char* src);
char* strchr(const char* s, int c);

void* memset(void* dest, int val, size_t count);
void* memcpy(void* dest, const void* src, size_t count);
void* memmove(void* dest, const void* src, size_t count);
int memcmp(const void* s1, const void* s2, size_t n);

int atoi(const char* s);
void itoa(int val, char* str, int base);
void utoa(uint32_t val, char* str, int base);

int vsprintf(char* buf, const char* fmt, __builtin_va_list args);
int sprintf(char* buf, const char* fmt, ...);

#endif
