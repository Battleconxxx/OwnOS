// fs.h
#ifndef FS_H
#define FS_H

#include <stddef.h>

#define MAX_FILES 128
#define MAX_NAME_LEN 64
#define MAX_FILE_SIZE 1024

typedef struct RamFile {
    char name[MAX_NAME_LEN];
    char data[MAX_FILE_SIZE];
    size_t size;
    int is_used;
} RamFile;

void ramfs_init();
int ramfs_create(const char* name);
int ramfs_write(const char* name, const char* data, size_t size);
int ramfs_read(const char* name, char* buffer, size_t size);
void ramfs_list();

#endif // FS_H