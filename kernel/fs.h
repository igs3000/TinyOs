#ifndef FS_H
#define FS_H

#include "types.h"

#define FS_MAX_FILES     64
#define FS_MAX_NAME      32
#define FS_MAX_CONTENT   8192

#define FS_FLAG_IN_USE   0x01
#define FS_FLAG_READONLY 0x02
#define FS_FLAG_SYSTEM   0x04

typedef struct {
    char     name[FS_MAX_NAME];
    uint32_t size;
    uint8_t  flags;
    char     timestamp[20];
    char     data[FS_MAX_CONTENT];
} file_entry_t;

void fs_init(void);
int fs_create(const char* name);
int fs_write(const char* name, const char* data, size_t len);
int fs_append(const char* name, const char* data, size_t len);
int fs_read(const char* name, char* buf, size_t max_len);
int fs_delete(const char* name);

file_entry_t* fs_get_file(const char* name);
file_entry_t* fs_get_file_by_index(int idx);
int fs_count(void);
uint32_t fs_total_bytes(void);
uint32_t fs_used_bytes(void);

#endif
