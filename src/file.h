
#ifndef FILE_H_INCLUDED
#define FILE_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>

#include "stretchy_buffer.h"

struct File;

struct File
{
	void *data;
	void *dataEnd;
	char *filename;
	char *shortname;
	size_t size;
	bool ownsData;
	bool ownsHandle; // if true, FileFree(file) will free(file)
};

bool FileExists(const char *filename);
struct File *FileNew(const char *filename, size_t size);
struct File *FileFromFilename(const char *filename);
struct File *FileFromData(void *data, size_t size, bool ownsData);
int FileToFilename(struct File *file, const char *filename);
const char *FileGetError(void);
void FileFree(struct File *file);
int FileSetError(const char *fmt, ...);
sb_array(char *, FileListFromDirectory)(const char *path, int depth, bool wantFiles, bool wantFolders, bool allocateIds);
sb_array(char *, FileListFilterBy)(sb_array(char *, list), const char *contains, const char *excludes);
sb_array(char *, FileListMergeVanilla)(sb_array(char *, list), sb_array(char *, vanilla));
sb_array(char *, FileListFilterByWithVanilla)(sb_array(char *, list), const char *contains, const char *vanilla);
sb_array(char *, FileListCopy)(sb_array(char *, list));
void FileListFree(sb_array(char *, list));
void FileListPrintAll(sb_array(char *, list));
int FileListFindIndexOfId(sb_array(char *, list), int id);
const char *FileListFindPathToId(sb_array(char *, list), int id);
sb_array(char *, FileListSortById)(sb_array(char *, list));
#define FILE_LIST_FILE_ID_PREFIX_LEN 2
#define FileListFilePrefix(STRING) \
	(((((const uint8_t*)(STRING)) - FILE_LIST_FILE_ID_PREFIX_LEN)[0] << 8) \
	| (((const uint8_t*)(STRING)) - FILE_LIST_FILE_ID_PREFIX_LEN)[1])

#endif // FILE_H_INCLUDED
