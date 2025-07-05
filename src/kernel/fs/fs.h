#pragma once
#include <stddef.h>

#define FS_MAX_FILES 64
#define FS_MAX_FILENAME 32
#define FS_MAX_FILESIZE 4096

typedef struct {
    char name[FS_MAX_FILENAME];
    size_t size;
    char data[FS_MAX_FILESIZE];
    int used;
} fs_file_t;

extern fs_file_t fs_files[FS_MAX_FILES];

int fs_create(const char *name);
int fs_delete(const char *name);
int fs_write(const char *name, const char *data, size_t size);
int fs_read(const char *name, char *buf, size_t bufsize);
int fs_list(char names[FS_MAX_FILES][FS_MAX_FILENAME], int *count);
int fs_exists(const char *name);
