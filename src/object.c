
#include <stdlib.h>
#include <stdio.h>

#include "object.h"
#include "misc.h"

static void ObjectParseAfterLoad(struct Object *obj)
{
	const struct File *file = obj->file;
	const uint8_t *start = obj->file->data;
	const uint8_t *end = obj->file->dataEnd;
	const uint32_t u24 = 0x00ffffff; // for masking lower 24 bits
	
	// find skeletons, starting 2 u32's from the end, backwards
	for (const uint8_t *walk = end - 8; walk > start; walk -= 4)
	{
		uint32_t skeletonHeaderAddr = walk - start; // where we are now
		int numLimbs = walk[4];
		uint32_t limbAddrsSegAddr = u32r(walk);
		
		// expects format nn000000
		if (u32r(walk + 4) & u24)
			continue;
		
		if (numLimbs == 0 || numLimbs > 127)
			continue;
		
		// not aligned
		if (limbAddrsSegAddr & 3)
			continue;
		
		const uint8_t *firstLimbAddr = start + (limbAddrsSegAddr & u24);
		
		// out of range
		if (firstLimbAddr + numLimbs * 4 > end)
			continue;
		
		// check whether is likely a list of limb addresses
		int i;
		int stride = 0;
		for (i = 0; i < numLimbs; ++i)
		{
			uint32_t limbAddr = u32r(firstLimbAddr + i * 4);
			uint32_t nextLimbAddr = u32r(firstLimbAddr + (i + 1) * 4);
			
			// points to some address beyond the EOF
			if ((limbAddr & u24) >= file->size)
				break;
			
			// unaligned
			if (limbAddr & 3)
				break;
			
			// sanity check last limb address
			if (i == numLimbs - 1)
			{
				// segment mismatch
				if ((limbAddr >> 24) != (limbAddrsSegAddr >> 24))
					break;
				
				// success
				i = numLimbs;
				break;
			}
			
			// initialize stride and sanity check it
			if (stride == 0)
			{
				stride = nextLimbAddr - limbAddr;
				
				// common strides
				if (stride != 12 && stride != 16)
					break;
			}
			
			// segment mismatch
			if ((limbAddr >> 24) != (nextLimbAddr >> 24))
				break;
			
			// every entry should be the same size
			if (nextLimbAddr - limbAddr != stride)
				break;
		}
		
		// didn't reach the end of the previous loop
		if (i < numLimbs)
			continue;
		
		// sanity check limbs themselves
		for (i = 0; i < numLimbs; ++i)
		{
			uint32_t limbAddr = u32r(firstLimbAddr + i * 4);
			const uint8_t *limb = start + (limbAddr & u24);
			uint32_t limbDL = u32r(limb + 8);
			
			// not aligned as DL's should be
			if (limbDL & 7)
				break;
			
			// allow ram addresses and blank limbs
			if ((limbDL >> 24) == 0x80
				|| limbDL == 0
			)
				continue;
			
			// don't allow segment mismatches
			if ((limbDL >> 24) != (limbAddrsSegAddr >> 24))
				break;
			
			// don't allow DL's that are beyond EOF
			if (start + (limbAddrsSegAddr & u24) >= end)
				break;
		}
		
		// didn't reach the end of the previous loop
		if (i < numLimbs)
			continue;
		
		// this may be a skeleton
		fprintf(stderr, "skeleton at %08x, %d limbs\n"
			, skeletonHeaderAddr, numLimbs
		);
	}
}

struct Object *ObjectFromFilename(const char *filename)
{
	struct Object *result = Calloc(1, sizeof(*result));
	
	result->file = FileFromFilename(filename);
	
	ObjectParseAfterLoad(result);
	
	return result;
}

void ObjectFree(struct Object *object)
{
	FileFree(object->file);
	
	free(object);
}
