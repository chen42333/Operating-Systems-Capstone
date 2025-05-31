#ifndef __FILE_H
#define __FILE_H

#include "utils.h"

int open(const char *pathname, int flags);
int close(int fd);
long write(int fd, const void *buf, unsigned long count);
long read(int fd, void *buf, unsigned long count);
int mkdir(const char *pathname, unsigned mode);
int mount(const char *src, const char *target, const char *filesystem, unsigned long flags, const void *data);
int chdir(const char *path);

void ls(char *dirname);
int cat(char *filename);
void* load_prog(char *filename, size_t *prog_size);

#endif