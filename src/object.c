
#include <stdlib.h>
#include <stdio.h>

#include "logging.h"
#include "object.h"
#include "misc.h"

static bool SegmentAddressIsValid(struct Object *obj, uint32_t segAddr, uint8_t alignment)
{
	const struct File *file = obj->file;
	const uint8_t *start = file->data;
	const uint8_t *end = file->dataEnd;
	const uint32_t u24 = 0x00ffffff; // for masking lower 24 bits
	
	return (
		(segAddr >> 24) == obj->segment
		&& !(segAddr & (alignment - 1))
		&& start + (segAddr & u24) < end
	);
}

static void ObjectParseAfterLoad(struct Object *obj)
{
	const struct File *file = obj->file;
	const uint8_t *start = obj->file->data;
	const uint8_t *end = obj->file->dataEnd;
	const uint8_t *endAligned8 = start + (obj->file->size & 0xfffffff8); // 8 byte aligned
	const uint32_t u24 = 0x00ffffff; // for masking lower 24 bits
	
	// find skeletons
	for (const uint8_t *walk = start; walk <= end - 8; walk += 4)
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
		LogDebug("skeleton at %08x, %d limbs"
			, skeletonHeaderAddr, numLimbs
		);
		sb_push(obj->skeletons, ((struct ObjectSkeleton){
			.limbAddrsSegAddr = limbAddrsSegAddr,
			.limbCount = numLimbs,
			.segAddr = skeletonHeaderAddr,
			.isLod = stride == 0x10,
		}));
	}
	
	// TODO use skeletons to derive datablobs
	// (that way, mesh/vertex/texture data is
	// excluded from the following searches)
	
	// search for animations
	for (const uint8_t *walk = start; walk <= end - 16; walk += 4)
	{
		struct ObjectAnimation anim = {
			.object = obj,
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
		LogDebug("animation at %08x, %d frames"
			, anim.segAddr, anim.numFrames
		);
		sb_push(obj->animations, anim);
	}
	
	// pre-baked pose for viewing player models
	if (
		sb_count(obj->skeletons) == 1
		&& sb_count(obj->animations) == 0
		&& obj->skeletons->limbCount == 21
		&& obj->skeletons->isLod
	)
	{
		static struct File pose;
		static struct Object poseObject;
		
		if (!pose.size)
		{
			static uint8_t poseData[] = {
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x0F, 0xA4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 0xF0, 0x5C, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0xE0, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x20, 0x00, 0x80, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x04, 0x00, 0x05,
				0x00, 0x06, 0x00, 0x07, 0x00, 0x08, 0x00, 0x09, 0x00, 0x0A, 0x00, 0x0B,
				0x00, 0x0C, 0x00, 0x0D, 0x00, 0x0E, 0x00, 0x0F, 0x00, 0x10, 0x00, 0x11,
				0x00, 0x12, 0x00, 0x13, 0x00, 0x14, 0x00, 0x15, 0x00, 0x16, 0x00, 0x17,
				0x00, 0x18, 0x00, 0x19, 0x00, 0x1A, 0x00, 0x1B, 0x00, 0x1C, 0x00, 0x1D,
				0x00, 0x1E, 0x00, 0x1F, 0x00, 0x20, 0x00, 0x21, 0x00, 0x22, 0x00, 0x23,
				0x00, 0x24, 0x00, 0x25, 0x00, 0x26, 0x00, 0x27, 0x00, 0x28, 0x00, 0x29,
				0x00, 0x2A, 0x00, 0x2B, 0x00, 0x2C, 0x00, 0x2D, 0x00, 0x2E, 0x00, 0x2F,
				0x00, 0x30, 0x00, 0x31, 0x00, 0x32, 0x00, 0x33, 0x00, 0x34, 0x00, 0x35,
				0x00, 0x36, 0x00, 0x37, 0x00, 0x38, 0x00, 0x39, 0x00, 0x3A, 0x00, 0x3B,
				0x00, 0x3C, 0x00, 0x3D, 0x00, 0x3E, 0x00, 0x3F, 0x00, 0x40, 0x00, 0x41,
			};
			pose.size = sizeof(poseData);
			pose.data = poseData;
			pose.dataEnd = poseData + pose.size;
			poseObject.file = &pose;
		}
		
		poseObject.segment = obj->segment;
		sb_push(obj->animations, ((struct ObjectAnimation) {
			.object = &poseObject,
			.segAddr = 0x06000000,
			.numFrames = 1,
			.limit = 3,
			.rotValSegAddr = 0x06000000,
			.rotIndexSegAddr = 0x06000084,
		}));
	}
	
	// rudimentary display list search
	for (const uint8_t *walk = endAligned8 - 8; walk >= start; walk -= 8)
	{
		uint32_t hi = u32r(walk);
		uint32_t lo = u32r(walk + 4);
		
		// these are the ways a dlist can end
		if (
			// explicit end of dlist
			(hi == (G_ENDDL << 24) && lo == 0)
			// end of dlist by way of hard branch
			|| (
				(hi == ((G_DL << 24) | 0x00010000))
				&& SegmentAddressIsValid(obj, lo, 8)
			)
		)
		{
			const int vbuf = 64; // only need 32 for f3dex2, but do 64 to support expanded microcodes
			bool containsGeometry = false;
			bool containsVertices = false;
			bool containsBranchToOtherDlist = false;
			
			// walk backwards and sanity check opcodes until you find the start
			for (walk -= 8; walk >= start; walk -= 8)
			{
				bool isValid = false;
				uint32_t hi = u32r(walk);
				uint32_t lo = u32r(walk + 4);
				
				switch (*walk)
				{
					case G_NOOP:
					case G_MODIFYVTX:
						// TODO these aren't typically used, but may add support for
						// them eventually as long as they don't create false positives
						break;
					
					case G_SPECIAL_3:
					case G_SPECIAL_2:
					case G_SPECIAL_1:
					case G_DMA_IO:
					case G_MOVEWORD:
					case G_MOVEMEM:
					case G_LOAD_UCODE:
					case G_SPNOOP:
					case G_TEXRECT:
					case G_TEXRECTFLIP:
					case G_SETKEYR:
					case G_SETKEYGB:
					case G_SETCONVERT:
					case G_SETSCISSOR:
					case G_SETPRIMDEPTH:
					case G_FILLRECT:
					case G_SETFILLCOLOR:
					case G_SETFOGCOLOR:
					case G_SETBLENDCOLOR:
					case G_SETZIMG:
					case G_SETCIMG:
						// XXX these are not used in any precompiled objects
						break;
					
					case G_RDPLOADSYNC:
					case G_RDPPIPESYNC:
					case G_RDPTILESYNC:
					case G_RDPFULLSYNC:
						isValid = (hi & u24) == 0 && lo == 0;
						break;
					
					// assume always valid for now, can add further sanity checks later if needed
					case G_TEXTURE:
					case G_GEOMETRYMODE:
					case G_RDPSETOTHERMODE:
					case G_SETPRIMCOLOR:
					case G_SETENVCOLOR:
					case G_SETCOMBINE:
					case G_SETTILESIZE: // could check tile descriptor on these
					case G_LOADBLOCK:
					case G_LOADTILE:
					case G_SETTILE:
						isValid = true;
						break;
					
					case G_SETTIMG: {
						// TODO could be more in-depth but this should be fine
						isValid = (
							walk[4] <= 0x0f
							&& !(walk[7] & 7)
						);
						if (walk[4] == obj->segment && (lo & u24) >= file->size)
							isValid = false;
						break;
					}
					
					case G_LOADTLUT:
						isValid = (
							(hi & u24) == 0
							&& (lo >> 28) == 0
							//&& walk[4] <= 7 // TODO should be max tile descriptor
							&& (lo & 0xfff) == 0
						);
						break;
					
					case G_POPMTX:
						isValid = (
							hi == ((G_POPMTX << 24) | 0x00380002)
							&& (
								lo == G_MTX_MODELVIEW
								|| lo == G_MTX_PROJECTION
							)
						);
						break;
					
					case G_MTX:
						isValid = (
							walk[1] == 0x38
							&& walk[2] == 0x00
							&& walk[3] <= (
								// TODO break into exclusive pairs in unlikely event false positives are an issue
								G_MTX_NOPUSH
								| G_MTX_PUSH
								| G_MTX_MUL
								| G_MTX_LOAD
								| G_MTX_MODELVIEW
								| G_MTX_PROJECTION
							)
							&& walk[4] <= 0x0f // other possible segments
							&& !(walk[7] & 3) // worst case scenario on alignment
							&& ((lo & u24) < file->size - 0x40) // end - sizeof(n64mtx)
						);
						break;
					
					case G_VTX: {
						uint16_t numv = (hi >> 12) & 0xfff;
						uint16_t vbidx = ((hi & 0xfff) >> 1) - numv;
						if (
							numv <= vbuf
							&& vbidx <= vbuf - numv // sanity check whether write would overflow
							&& SegmentAddressIsValid(obj, lo, 8) // technically only need align=4
						) {
							isValid = true;
							containsVertices = true;
						}
						break;
					}
					case G_CULLDL: {
						// 0300vvvv 0000wwww
						uint16_t vfirst = hi & 0xffff;
						uint16_t vlast = lo & 0xffff;
						isValid = (
							!walk[1]
							&& !(lo >> 16)
							&& vfirst < vlast
							&& 0 <= vfirst // range 0 <= n < 32
							&& vfirst < vbuf
							&& 0 <= vlast // range 0 <= n < 32
							&& vlast < vbuf
						);
						break;
					}
					case G_BRANCH_Z: {
						// E1000000 dddddddd
						// 04aaabbb zzzzzzzz
						// doesn't have to be immediately preceded by E1, because that
						// register could always be set elsewhere, but in precompiled
						// dl's, it always seems to be (also checking this way simplifies
						// this test and reduces false positives)
						uint16_t a = (hi >> 12) & 0xfff;
						uint16_t b = hi & 0xfff;
						if (walk - 8 >= start
							&& u32r(walk - 8) == (G_RDPHALF_1 << 24)
							&& SegmentAddressIsValid(obj, u32r(walk - 4), 8)
							&& (a * 2 == b * 5) // TODO check this math against real opcodes
						)
							isValid = true;
						break;
					}
					case G_TRI1: {
						// 05aabbcc 00000000
						containsGeometry = true;
						isValid = (
							lo == 0
							&& !(hi & 0x010101) // abc = even numbers
							&& walk[1] / 2 < vbuf
							&& walk[2] / 2 < vbuf
							&& walk[3] / 2 < vbuf
						);
						break;
					}
					case G_QUAD: // quad layout is mostly interchangeable for tri2
					case G_TRI2: {
						// 06aabbcc 00ddeeff
						containsGeometry = true;
						isValid = (
							walk[4] == 0
							&& !(hi & 0x010101) // abc = even numbers
							&& !(lo & 0x010101) // def = even numbers
							&& walk[1] / 2 < vbuf
							&& walk[2] / 2 < vbuf
							&& walk[3] / 2 < vbuf
							&& walk[5] / 2 < vbuf
							&& walk[6] / 2 < vbuf
							&& walk[7] / 2 < vbuf
						);
						break;
					}
					case G_RDPHALF_1:
					case G_RDPHALF_2:
						isValid = !(hi & u24);
						break;
					case G_SETOTHERMODE_L:
					case G_SETOTHERMODE_H: {
						// TODO could try sanity checking shift/length params
						isValid = walk[1] == 0;
						break;
					}
					
					// these are valid opcodes, but when walking backwards through a
					// dlist, encountering them means we have underflowed into the end
					// of the dlist preceding the current one, aka our end condition
					case G_ENDDL:
						isValid = false;
						break;
					case G_DL:
						if (walk[1]) // hard branch
							isValid = false;
						else if (walk[4] > 0x0f || (walk[7] & 7))
							isValid = false;
						else
							isValid = true;
						if (SegmentAddressIsValid(obj, lo, 8))
							containsBranchToOtherDlist = true;
						break;
				}
				
				// underflowed 8 bytes before start of dlist, or start of file
				if (!isValid || walk == start)
				{
					if (!isValid)
						walk += 8;
					
					// let's only note dlists that can be rendered in 3d
					if ((containsVertices && containsGeometry) || containsBranchToOtherDlist)
					{
						sb_push(obj->meshes, (struct ObjectMesh) {
							.segAddr = (obj->segment << 24) | (walk - start)
						});
					}
					
					break;
				}
			}
		}
	}
	sb_reverse(obj->meshes);
	
	// report findings
	sb_foreach(obj->meshes, { LogDebug("found DL %08x", each->segAddr); });
}

struct Object *ObjectFromFilename(const char *filename, int segment)
{
	struct Object *result = Calloc(1, sizeof(*result));
	
	result->file = FileFromFilename(filename);
	
	// automatic segment detection
	if (segment <= 0)
	{
		int counts[N64_SEGMENT_MAX];
		struct File *file = result->file;
		uint8_t *end = file->dataEnd;
		
		memset(counts, 0, sizeof(counts));
		for (uint8_t *walk = file->data; walk <= end - 16; walk += 8)
		{
			const uint8_t thisCmd = walk[0];
			const uint8_t nextCmd = walk[8];
			
			// vertex load followed by triangle draw
			if (thisCmd == G_VTX && (nextCmd == G_TRI1 || nextCmd == G_TRI2))
			{
				// try validating
				const int vbuf = 64;
				const uint8_t thisSeg = walk[4];
				uint32_t hi = u32r(walk);
				uint32_t lo = u32r(walk + 4);
				uint16_t numv = (hi >> 12) & 0xfff;
				uint16_t vbidx = ((hi & 0xfff) >> 1) - numv;
				bool containsVertices = (
					numv <= vbuf
					&& vbidx <= vbuf - numv // sanity check whether write would overflow
					&& (result->segment = thisSeg) < N64_SEGMENT_MAX
					&& SegmentAddressIsValid(result, lo, 8) // technically only need align=4
				);
				bool containsGeometry = (
					walk += 8
					&& (hi = u32r(walk))
					&& (lo = u32r(walk + 4))
					&& walk[4] == 0
					&& !(hi & 0x010101) // abc = even numbers
					&& !(lo & 0x010101) // def = even numbers
					&& walk[1] / 2 < vbuf
					&& walk[2] / 2 < vbuf
					&& walk[3] / 2 < vbuf
					&& (
						(nextCmd == G_TRI1 && !lo) // 05aabbcc 00000000
						|| (nextCmd == G_TRI2 // 06aabbcc 00ddeeff
							&& walk[5] / 2 < vbuf
							&& walk[6] / 2 < vbuf
							&& walk[7] / 2 < vbuf
						)
					)
				);
				
				// increment the counter
				if (containsVertices && containsGeometry)
					counts[thisSeg] += 1;
			}
		}
		
		// whichever segment is most common will be the one
		segment = ArrayGetIndexofMaxInt(counts, N64_ARRAY_COUNT(counts));
		
		// fall back to default if no matches
		if (segment < 0 || !counts[segment])
			segment = 0x06;
	}
	
	result->segment = segment;
	
	ObjectParseAfterLoad(result);
	
	return result;
}

void ObjectFree(struct Object *object)
{
	FileFree(object->file);
	
	sb_free(object->meshes);
	sb_free(object->animations);
	sb_free(object->skeletons);
	
	free(object);
}
