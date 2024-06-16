
#include <stdlib.h>
#include <stdio.h>

#include "object.h"
#include "misc.h"

struct Object *ObjectFromFilename(const char *filename)
{
	struct Object *result = Calloc(1, sizeof(*result));
	
	result->file = FileFromFilename(filename);
	
	return result;
}

void ObjectFree(struct Object *object)
{
	FileFree(object->file);
	
	free(object);
}
