#include <stdio.h>

#include <stdio.h>
#include <stdarg.h>

#include "misc.h"
#include "file.h"

static char sFileError[2048];

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
