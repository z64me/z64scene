
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
		uint32_t skeletonHeaderAddr = (walk - start) | (obj->segment << 24);
		int numLimbs = walk[4];
		uint32_t limbAddrsSegAddr = u32r(walk);
		
		// assert common segments
		if ((limbAddrsSegAddr >> 24) < 0x04
			|| (limbAddrsSegAddr >> 24) > 0x06
		)
			continue;
		
		// let's assume skeletons don't store
		// their limb lists in other segments
		if ((limbAddrsSegAddr >> 24) != obj->segment)
			continue;
		
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
			
			// don't allow bad segments
			if ((limbDL >> 24) > 0x0F
				|| (limbDL >> 24) == 0x00
			)
				break;
			
			// don't allow DL's that are beyond EOF
			if ((limbDL >> 24) == (limbAddrsSegAddr >> 24)
				&& start + (limbAddrsSegAddr & u24) >= end
			)
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
	
	// TODO use skeletons to derive datablobs
	// (that way, mesh/vertex/texture data is
	// excluded from the following searches)
	
	// search for animations
	for (const uint8_t *walk = start; walk <= end - 16; walk += 4)
	{
		struct ObjectAnimation anim = {
			.numFrames = u16r(walk + 0),
			.pad0 = u16r(walk + 2),
			.rotValSegAddr = u32r(walk + 4),
			.rotIndexSegAddr = u32r(walk + 8),
			.limit = u16r(walk + 12),
			.pad1 = u16r(walk + 14),
			.segAddr = (walk - start) | (obj->segment << 24),
		};
		
		// these are expected to always be 0
		if (anim.pad0 || anim.pad1)
			continue;
		
		// these are expected to always be non-zero
		if (!anim.rotValSegAddr || !anim.rotIndexSegAddr || !anim.numFrames)
			continue;
		
		// reasonable to expect no animation is this long
		if (anim.numFrames >= 500)
			continue;
		
		// mismatching segments
		if ((anim.rotValSegAddr >> 24) != (anim.rotIndexSegAddr >> 24))
			continue;
		
		// bad segment in general
		if ((anim.rotValSegAddr >> 24) > 0x0F
			|| (anim.rotValSegAddr >> 24) == 0x00
		)
			continue;
		
		// let's assume animations don't reference other segments
		// (do they ever?)
		if ((anim.rotValSegAddr >> 24) != obj->segment)
			continue;
		
		// each value is 16 bit, so should be 2-byte-aligned
		if ((anim.rotValSegAddr & 1) || (anim.rotIndexSegAddr & 1))
			continue;
		
		//
		// TODO find out whether this check would be okay to include
		//
		// for extract safety, you could also check whether the first
		// u16 in rotValSegAddr is 0, because it seems to be for most
		// (all?) animations, but it would come at the cost of false
		// negatives (aka skipping over real animations)
		//
		
		// is likely an animation
		fprintf(stderr, "animation at %08x, %d frames\n"
			, anim.segAddr, anim.numFrames
		);
		sb_push(obj->animations, anim);
	}
}

struct Object *ObjectFromFilename(const char *filename)
{
	struct Object *result = Calloc(1, sizeof(*result));
	
	result->file = FileFromFilename(filename);
	
	result->segment = 0x06; // is fine hard-coded for now
	
	ObjectParseAfterLoad(result);
	
	return result;
}

void ObjectFree(struct Object *object)
{
	FileFree(object->file);
	
	free(object);
}
