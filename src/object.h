
#ifndef OBJECT_H_INCLUDED
#define OBJECT_H_INCLUDED

#include <stdint.h>

#include "file.h"
#include "stretchy_buffer.h"

struct Object;

struct ObjectAnimation
{
	const struct Object *object;
	/* 0x0000 */ uint16_t numFrames;
	/* 0x0002 */ uint16_t pad0;
	/* 0x0004 */ uint32_t rotValSegAddr;
	/* 0x0008 */ uint32_t rotIndexSegAddr;
	/* 0x000C */ uint16_t limit;
	/* 0x000E */ uint16_t pad1;
	
	uint32_t segAddr;
};

struct ObjectSkeleton
{
	/* 0x0000 */ uint32_t limbAddrsSegAddr;
	/* 0x0004 */ uint8_t limbCount;
	/* 0x0008 */ uint8_t limbMatrixCount; // unused
	
	uint32_t segAddr;
};

struct Object
{
	struct File *file;
	uint8_t segment;
	
	sb_array(struct ObjectSkeleton, skeletons);
	sb_array(struct ObjectAnimation, animations);
};

struct Object *ObjectFromFilename(const char *filename, int segment);
void ObjectFree(struct Object *object);

#endif // OBJECT_H_INCLUDED
