#include "fs.h"
#include <libk/string/string.h>

fs_file_t fs_files[FS_MAX_FILES] = {0};

int fs_create(const char *name) {
    if (fs_exists(name)) return -1;
    for (int i = 0; i < FS_MAX_FILES; ++i) {
        if (!fs_files[i].used) {
            strncpy(fs_files[i].name, name, FS_MAX_FILENAME-1);
            fs_files[i].name[FS_MAX_FILENAME-1] = 0;
            fs_files[i].size = 0;
            fs_files[i].used = 1;
            return 0;
        }
    }
    return -1;
}

int fs_delete(const char *name) {
    for (int i = 0; i < FS_MAX_FILES; ++i) {
        if (fs_files[i].used && strcmp(fs_files[i].name, name) == 0) {
            fs_files[i].used = 0;
            return 0;
        }
    }
    return -1;
}

int fs_write(const char *name, const char *data, size_t size) {
    for (int i = 0; i < FS_MAX_FILES; ++i) {
        if (fs_files[i].used && strcmp(fs_files[i].name, name) == 0) {
            size_t to_copy = size > FS_MAX_FILESIZE ? FS_MAX_FILESIZE : size;
            memcpy(fs_files[i].data, data, to_copy);
            fs_files[i].size = to_copy;
            return 0;
        }
    }
    return -1;
}

int fs_read(const char *name, char *buf, size_t bufsize) {
    for (int i = 0; i < FS_MAX_FILES; ++i) {
        if (fs_files[i].used && strcmp(fs_files[i].name, name) == 0) {
            size_t to_copy = fs_files[i].size > bufsize ? bufsize : fs_files[i].size;
            memcpy(buf, fs_files[i].data, to_copy);
            return (int)to_copy;
        }
    }
    return -1;
}

int fs_list(char names[FS_MAX_FILES][FS_MAX_FILENAME], int *count) {
    int n = 0;
    for (int i = 0; i < FS_MAX_FILES; ++i) {
        if (fs_files[i].used) {
            strncpy(names[n++], fs_files[i].name, FS_MAX_FILENAME-1);
            names[n-1][FS_MAX_FILENAME-1] = 0;
        }
    }
    *count = n;
    return 0;
}

int fs_exists(const char *name) {
    for (int i = 0; i < FS_MAX_FILES; ++i)
        if (fs_files[i].used && strcmp(fs_files[i].name, name) == 0)
            return 1;
    return 0;
}
