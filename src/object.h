
#ifndef OBJECT_H_INCLUDED
#define OBJECT_H_INCLUDED

#include <stdint.h>

#include "file.h"

struct Object
{
	struct File *file;
};

struct Object *ObjectFromFilename(const char *filename);
void ObjectFree(struct Object *object);

#endif // OBJECT_H_INCLUDED
