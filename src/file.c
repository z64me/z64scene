#define _XOPEN_SOURCE 500 // nftw

#include <stdio.h>

#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <ftw.h>

#include "misc.h"
#include "file.h"

// define prefix type w/ guaranteed binary prefix
#define FILE_LIST_DEFINE_PREFIX(X) \
	static char X##Data[] = "\0\0" #X; \
	static char *X = X##Data + FILE_LIST_FILE_ID_PREFIX_LEN;

static char sFileError[2048];
FILE_LIST_DEFINE_PREFIX(FileListHasPrefixId)
FILE_LIST_DEFINE_PREFIX(FileListAttribIsHead) // owns the strings it references
#define FILE_LIST_ON_PREFIX(STRING, CODE) \
	if ((STRING) == FileListHasPrefixId \
		|| (STRING) == FileListAttribIsHead \
	) { CODE; }

bool FileExists(const char *filename)
{
	FILE *fp = fopen(filename, "rb");
	
	if (!fp)
		return false;
	
	fclose(fp);
	
	return true;
}

struct File *FileNew(const char *filename, size_t size)
{
	struct File *result = Calloc(1, sizeof(*result));
	
	result->size = size;
	result->filename = Strdup(filename);
	if (result->filename)
		result->shortname = Strdup(MAX3(
			strrchr(result->filename, '/') + 1
			, strrchr(result->filename, '\\') + 1
			, result->filename
		));
	result->data = Calloc(1, size);
	result->dataEnd = ((uint8_t*)result->data) + result->size;
	
	return result;
}

struct File *FileFromFilename(const char *filename)
{
	struct File *result = Calloc(1, sizeof(*result));
	FILE *fp;
	
	if (!filename || !*filename) Die("empty filename");
	if (!(fp = fopen(filename, "rb"))) Die("failed to open '%s' for reading", filename);
	if (fseek(fp, 0, SEEK_END)) Die("error moving read head '%s'", filename);
	if (!(result->size = ftell(fp))) Die("error reading '%s', empty?", filename);
	if (fseek(fp, 0, SEEK_SET)) Die("error moving read head '%s'", filename);
	result->data = Calloc(1, result->size + 1); // zero terminator on strings
	result->dataEnd = ((uint8_t*)result->data) + result->size;
	if (fread(result->data, 1, result->size, fp) != result->size)
		Die("error reading contents of '%s'", filename);
	if (fclose(fp)) Die("error closing file '%s' after reading", filename);
	result->filename = Strdup(filename);
	result->shortname = Strdup(MAX3(
		strrchr(filename, '/') + 1
		, strrchr(filename, '\\') + 1
		, filename
	));
	
	return result;
}

const char *FileGetError(void)
{
	return sFileError;
}

int FileToFilename(struct File *file, const char *filename)
{
	FILE *fp;
	
	if (!filename || !*filename) return FileSetError("empty filename");
	if (!file) return FileSetError("empty error");
	if (!(fp = fopen(filename, "wb")))
		return FileSetError("failed to open '%s' for writing", filename);
	if (fwrite(file->data, 1, file->size, fp) != file->size)
		return FileSetError("failed to write full contents of '%s'", filename);
	if (fclose(fp))
		return FileSetError("error closing file '%s' after writing", filename);
	
	return EXIT_SUCCESS;
}

int FileSetError(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	
	vsnprintf(sFileError, sizeof(sFileError), fmt, args);
	
	va_end(args);
	
	return EXIT_FAILURE;
}

void FileListFree(sb_array(char *, list))
{
	// if it owns the strings it references, free non-prefix strings
	if (sb_contains_ref(list, FileListAttribIsHead))
	{
		bool hasPrefixId = sb_contains_ref(list, FileListHasPrefixId);
		int padEach = hasPrefixId ? -FILE_LIST_FILE_ID_PREFIX_LEN : 0;
		
		sb_foreach(list, {
			FILE_LIST_ON_PREFIX(*each, continue)
			free((*each) + padEach);
		})
	}
	
	sb_free(list);
}

sb_array(char *, FileListFromDirectory)(const char *path, bool wantFiles, bool wantFolders, bool allocateIds)
{
	sb_array(char *, result) = 0;
	int padEach = 0;
	
	// binary prefix for each file
	if (allocateIds)
	{
		sb_push(result, FileListHasPrefixId);
		padEach = -FILE_LIST_FILE_ID_PREFIX_LEN;
	}
	
	// this file list owns the strings it references
	sb_push(result, FileListAttribIsHead);
	
	int each(const char *pathname, const struct stat *sbuf, int type, struct FTW *ftwb)
	{
		// filter by request
		if (!(
			(wantFiles && type == FTW_F)
			|| (wantFolders
				&& (type == FTW_D
					|| type == FTW_DNR
					|| type == FTW_DP
					|| type == FTW_SL
				)
			)
		))
			return 0;
		
		// skip extensionless files
		if (type == FTW_F && !strchr(MAX3(
				strrchr(pathname, '/') + 1
				, strrchr(pathname, '\\') + 1
				, pathname
			), '.')
		)
			return 0;
		
		// consistency on the slashes
		char *copy = StrdupPad(pathname, padEach);
		for (char *c = copy; *c; ++c)
			if (*c == '\\')
				*c = '/';
		
		// add to list
		sb_push(result, copy);
		
		return 0;
		
		// -Wunused-parameter
		(void)sbuf;
		(void)ftwb;
	}
	
	if (nftw(path, each, 64 /* max directory depth */, FTW_DEPTH) < 0)
		Die("failed to walk file tree '%s'", path);
	
	return result;
}

sb_array(char *, FileListFilterBy)(sb_array(char *, list), const char *contains, const char *excludes)
{
	sb_array(char *, result) = 0;
	bool hasPrefixId = sb_contains_ref(list, FileListHasPrefixId);
	
	if (hasPrefixId)
		sb_push(result, FileListHasPrefixId);
	
	sb_foreach(list, {
		char *str = *each;
		char *match = 0;
		if (contains && !(match = strstr(str, contains)))
			continue;
		if (excludes && strstr(str, excludes))
			continue;
		
		if (match)
		{
			match += strlen(contains);
			while (!isalnum(*match)) ++match;
			
			// add id prefix
			int v;
			if (hasPrefixId && sscanf(match, "%i", &v) == 1)
			{
				uint8_t *prefix = (void*)(str - 2);
				prefix[0] = (v >> 8);
				prefix[1] = v & 0xff;
			}
			
			// ignore subdirectories
			if (strchr(match, '/'))
				continue;
		}
		
		sb_push(result, str);
	});
	
	return result;
}

sb_array(char *, FileListCopy)(sb_array(char *, list))
{
	int count = sb_count(list);
	sb_array(char *, result) = 0;
	
	if (!count)
		return result;
	
	(void)sb_add(result, count);
	
	memcpy(result, list, count * sizeof(*list));
	
	return result;
}

sb_array(char *, FileListMergeVanilla)(sb_array(char *, list), sb_array(char *, vanilla))
{
	sb_array(char *, result) = FileListCopy(list);
	bool hasPrefixId = sb_contains_ref(list, FileListHasPrefixId);
	
	// compare id's = faster
	if (hasPrefixId)
	{
		sb_foreach(vanilla, {
			uint16_t vanillaPrefix = FileListFilePrefix(*each);
			if (vanillaPrefix == 0)
				continue;
			bool skip = false;
			sb_foreach(list, {
				if (FileListFilePrefix(*each) == vanillaPrefix) {
					skip = true;
					break;
				}
			})
			if (skip == false)
				sb_push(result, *each);
		})
		
		return result;
	}
	
	// include vanilla folders whose id's don't match mod folder id's
	sb_foreach(vanilla, {
		const char *needle = strrchr(*each, '/');
		if (!needle)
			continue;
		int len = strcspn(needle, "- ");
		bool skip = false;
		sb_foreach(list, {
			const char *haystack = strrchr(*each, '/');
			if (!haystack)
				continue;
			if (!strncmp(needle, haystack, len)) {
				skip = true;
				break;
			}
		})
		if (skip == false)
			sb_push(result, *each);
	})
	
	return result;
}

sb_array(char *, FileListFilterByWithVanilla)(sb_array(char *, list), const char *contains, const char *vanilla)
{
	char filterA[64];
	char filterB[64];
	char vanillaSlash[64];
	
	if (!vanilla || !*vanilla)
	{
		snprintf(filterA, sizeof(filterA), "/%s/", contains);
		return FileListFilterBy(list, filterA, 0);
	}
	
	snprintf(filterA, sizeof(filterA), "/%s/", contains);
	snprintf(filterB, sizeof(filterB), "/%s/%s/", contains, vanilla);
	snprintf(vanillaSlash, sizeof(vanillaSlash), "/%s/", vanilla);
	
	sb_array(char *, objectsVanilla) = FileListFilterBy(list, filterB, 0);
	sb_array(char *, objects) = FileListFilterBy(list, filterA, vanillaSlash);
	sb_array(char *, merged) = FileListMergeVanilla(objects, objectsVanilla);
	
	//fprintf(stderr, "objectsVanilla = %p\n", objectsVanilla);
	//fprintf(stderr, "objects = %p\n", objects);
	//fprintf(stderr, "merged = %p\n", merged);
	//FileListPrintAll(merged);
	
	sb_free(objectsVanilla);
	sb_free(objects);
	
	return merged;
}

void FileListPrintAll(sb_array(char *, list))
{
	// prefixes
	if (sb_contains_ref(list, FileListHasPrefixId))
	{
		sb_foreach(list, {
			fprintf(stderr, "%s, prefix=0x%04x\n", *each, FileListFilePrefix(*each));
		})
		return;
	}
	
	sb_foreach(list, {
		fprintf(stderr, "%s\n", *each);
	})
}
