
#ifndef FILE_H_INCLUDED
#define FILE_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>

struct File;

struct File
{
	void *data;
	void *dataEnd;
	char *filename;
	char *shortname;
	size_t size;
};

bool FileExists(const char *filename);
struct File *FileNew(const char *filename, size_t size);
struct File *FileFromFilename(const char *filename);
int FileToFilename(struct File *file, const char *filename);
const char *FileGetError(void);
void FileFree(struct File *file);
int FileSetError(const char *fmt, ...);

#endif // FILE_H_INCLUDED
