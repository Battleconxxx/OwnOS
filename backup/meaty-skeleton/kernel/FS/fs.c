// fs.c
#include <string.h>
#include <stdio.h>
#include <kernel/fs.h>

static RamFile files[MAX_FILES];

void ramfs_init() {
    for (int i = 0; i < MAX_FILES; ++i) {
        files[i].is_used = 0;
    }
}

int ramfs_create(const char* name) {
    for (int i = 0; i < MAX_FILES; ++i) {
        if (!files[i].is_used) {
            strncpy(files[i].name, name, MAX_NAME_LEN);
            files[i].size = 0;
            files[i].is_used = 1;
            return 0;
        }
    }
    return -1;
}

int ramfs_write(const char* name, const char* data, size_t size) {
    for (int i = 0; i < MAX_FILES; ++i) {
        if (files[i].is_used && strncmp(files[i].name, name, MAX_NAME_LEN) == 0) {
            if (size > MAX_FILE_SIZE) return -1;
            memcpy(files[i].data, data, size);
            files[i].size = size;
            return 0;
        }
    }
    return -1;
}

int ramfs_read(const char* name, char* buffer, size_t size) {
    for (int i = 0; i < MAX_FILES; ++i) {
        if (files[i].is_used && strncmp(files[i].name, name, MAX_NAME_LEN) == 0) {
            size_t read_size = (size < files[i].size) ? size : files[i].size;
            memcpy(buffer, files[i].data, read_size);
            return (int)read_size;
        }
    }
    return -1;
}

void ramfs_list() {
    printf("Files in RAMFS:\n");
    for (int i = 0; i < MAX_FILES; ++i) {
        if (files[i].is_used) {
            printf("- %s (%zu bytes)\n", files[i].name, files[i].size);
        }
    }
}