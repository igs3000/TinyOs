#include "string.h"

size_t strlen(const char* s) {
    size_t len = 0;
    while (s[len]) {
        len++;
    }
    return len;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}

char* strcat(char* dest, const char* src) {
    char* d = dest + strlen(dest);
    while ((*d++ = *src++));
    return dest;
}

char* strchr(const char* s, int c) {
    while (*s) {
        if (*s == (char)c) return (char*)s;
        s++;
    }
    if ((char)c == '\0') return (char*)s;
    return NULL;
}

void* memset(void* dest, int val, size_t count) {
    unsigned char* d = (unsigned char*)dest;
    unsigned char v = (unsigned char)val;
    for (size_t i = 0; i < count; i++) {
        d[i] = v;
    }
    return dest;
}

void* memcpy(void* dest, const void* src, size_t count) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    for (size_t i = 0; i < count; i++) {
        d[i] = s[i];
    }
    return dest;
}

void* memmove(void* dest, const void* src, size_t count) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    if (d < s) {
        for (size_t i = 0; i < count; i++) {
            d[i] = s[i];
        }
    } else if (d > s) {
        for (size_t i = count; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    }
    return dest;
}

int memcmp(const void* s1, const void* s2, size_t n) {
    const unsigned char* p1 = (const unsigned char*)s1;
    const unsigned char* p2 = (const unsigned char*)s2;
    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }
    return 0;
}

int atoi(const char* s) {
    int res = 0;
    int sign = 1;
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+') {
        s++;
    }
    while (*s >= '0' && *s <= '9') {
        res = res * 10 + (*s - '0');
        s++;
    }
    return res * sign;
}

void itoa(int val, char* str, int base) {
    char* ptr = str;
    char* ptr1 = str;
    char tmp_char;
    int tmp_val;

    if (base < 2 || base > 36) {
        *str = '\0';
        return;
    }

    if (val < 0 && base == 10) {
        *ptr++ = '-';
        val = -val;
        ptr1++;
    }

    tmp_val = val;
    do {
        int rem = tmp_val % base;
        *ptr++ = (rem < 10) ? (rem + '0') : (rem - 10 + 'a');
        tmp_val /= base;
    } while (tmp_val);

    *ptr-- = '\0';
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
}

void utoa(uint32_t val, char* str, int base) {
    char* ptr = str;
    char* ptr1 = str;
    char tmp_char;
    uint32_t tmp_val;

    if (base < 2 || base > 36) {
        *str = '\0';
        return;
    }

    tmp_val = val;
    do {
        uint32_t rem = tmp_val % (uint32_t)base;
        *ptr++ = (rem < 10) ? (rem + '0') : (rem - 10 + 'a');
        tmp_val /= (uint32_t)base;
    } while (tmp_val);

    *ptr-- = '\0';
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
}

int vsprintf(char* buf, const char* fmt, __builtin_va_list args) {
    char* p = buf;
    char num_buf[32];

    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            // Check flags / width if needed
            int zero_pad = 0;
            int width = 0;
            if (*fmt == '0') {
                zero_pad = 1;
                fmt++;
            }
            while (*fmt >= '0' && *fmt <= '9') {
                width = width * 10 + (*fmt - '0');
                fmt++;
            }

            switch (*fmt) {
                case 'c': {
                    char c = (char)__builtin_va_arg(args, int);
                    *p++ = c;
                    break;
                }
                case 's': {
                    const char* s = __builtin_va_arg(args, const char*);
                    if (!s) s = "(null)";
                    while (*s) {
                        *p++ = *s++;
                    }
                    break;
                }
                case 'd':
                case 'i': {
                    int n = __builtin_va_arg(args, int);
                    itoa(n, num_buf, 10);
                    int len = (int)strlen(num_buf);
                    while (len < width) {
                        *p++ = zero_pad ? '0' : ' ';
                        width--;
                    }
                    char* s = num_buf;
                    while (*s) *p++ = *s++;
                    break;
                }
                case 'u': {
                    uint32_t u = __builtin_va_arg(args, uint32_t);
                    utoa(u, num_buf, 10);
                    int len = (int)strlen(num_buf);
                    while (len < width) {
                        *p++ = zero_pad ? '0' : ' ';
                        width--;
                    }
                    char* s = num_buf;
                    while (*s) *p++ = *s++;
                    break;
                }
                case 'x': {
                    uint32_t x = __builtin_va_arg(args, uint32_t);
                    utoa(x, num_buf, 16);
                    int len = (int)strlen(num_buf);
                    while (len < width) {
                        *p++ = zero_pad ? '0' : ' ';
                        width--;
                    }
                    char* s = num_buf;
                    while (*s) *p++ = *s++;
                    break;
                }
                case 'X': {
                    uint32_t x = __builtin_va_arg(args, uint32_t);
                    utoa(x, num_buf, 16);
                    for (int i = 0; num_buf[i]; i++) {
                        if (num_buf[i] >= 'a' && num_buf[i] <= 'f') {
                            num_buf[i] -= 32;
                        }
                    }
                    int len = (int)strlen(num_buf);
                    while (len < width) {
                        *p++ = zero_pad ? '0' : ' ';
                        width--;
                    }
                    char* s = num_buf;
                    while (*s) *p++ = *s++;
                    break;
                }
                case 'p': {
                    uint32_t pval = (uint32_t)__builtin_va_arg(args, void*);
                    *p++ = '0';
                    *p++ = 'x';
                    utoa(pval, num_buf, 16);
                    int len = (int)strlen(num_buf);
                    while (len < 8) {
                        *p++ = '0';
                        len++;
                    }
                    char* s = num_buf;
                    while (*s) *p++ = *s++;
                    break;
                }
                case '%': {
                    *p++ = '%';
                    break;
                }
                default:
                    *p++ = '%';
                    *p++ = *fmt;
                    break;
            }
        } else {
            *p++ = *fmt;
        }
        fmt++;
    }
    *p = '\0';
    return (int)(p - buf);
}

int sprintf(char* buf, const char* fmt, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    int ret = vsprintf(buf, fmt, args);
    __builtin_va_end(args);
    return ret;
}
