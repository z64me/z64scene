
#include "misc.h"
#include "skelanime.h"
#include "object.h"
#include "logging.h"

#include <string.h>
#include <stdio.h>

#include <n64.h>

#define SEGMENTED_TO_VIRTUAL(X) n64_segment_get(X)

static inline stb_sb_find_impl(FindLimbOverride, const struct ObjectLimbOverride, int, each->limbIndex == needle)

void SkelAnime_Init(SkelAnime *this, const struct Object *object, const struct ObjectSkeleton *skeleton, const struct ObjectAnimation *animation)
{
	// don't reinitialize if nothing changed
	if (this->object == object
		&& this->skeleton == skeleton
		&& this->animation == animation
	)
		return;
	
	memset(this, 0, sizeof(*this));
	
	this->object = object;
	this->skeleton = skeleton;
	this->animation = animation;
	this->limbCount = skeleton->limbCount + 1;
	this->playSpeed = 1.0f;
}

void SkelAnime_Free(SkelAnime* this)
{
	memset(this, 0, sizeof(*this));
}

static void SkelAnime_GetFrameData(const struct ObjectAnimation *anim, int frame, int limbCount, Vec3s* frameTable)
{
	const Vec3s *jointIndices = SEGMENTED_TO_VIRTUAL(anim->rotIndexSegAddr);
	const int16_t *frameData = SEGMENTED_TO_VIRTUAL(anim->rotValSegAddr);
	const int16_t *staticData = &frameData[0];
	const int16_t *dynamicData = &frameData[frame];
	uint16_t limit = anim->limit;
	
	if (!jointIndices || !frameData)
		Die("SkelAnime_GetFrameData() failed on %08x %08x",
			anim->rotIndexSegAddr, anim->rotValSegAddr
		);
	
	for (int i = 0; i < limbCount; ++i, ++frameTable, ++jointIndices)
	{
		Vec3s swapInd = { u16r3(jointIndices) };
		
		frameTable->x = u16r(swapInd.x >= limit ? &dynamicData[swapInd.x] : &staticData[swapInd.x]);
		frameTable->y = u16r(swapInd.y >= limit ? &dynamicData[swapInd.y] : &staticData[swapInd.y]);
		frameTable->z = u16r(swapInd.z >= limit ? &dynamicData[swapInd.z] : &staticData[swapInd.z]);
	}
}

static void SkelAnime_InterpFrameTable(int limbCount, Vec3s* dst, Vec3s* start, Vec3s* target, float weight)
{
	if (weight < 1.0f)
	{
		int16_t diff;
		int16_t base;
		
		for (int i = 0; i < limbCount; ++i, ++dst, ++start, ++target)
		{
			base = start->x;
			diff = target->x - base;
			dst->x = (s16)(diff * weight) + base;
			base = start->y;
			diff = target->y - base;
			dst->y = (s16)(diff * weight) + base;
			base = start->z;
			diff = target->z - base;
			dst->z = (s16)(diff * weight) + base;
		}
	}
	else
	{
		for (int i = 0; i < limbCount; ++i, ++dst, ++target)
		{
			dst->x = target->x;
			dst->y = target->y;
			dst->z = target->z;
		}
	}
}

void SkelAnime_Update(SkelAnime* this, double deltaTimeFrames)
{
	const struct ObjectAnimation *anim = this->animation;
	const struct Object *obj = anim->object;
	
	// frozen on frame 0 in an animation playing in reverse = ensure signbit is set
	// (frame '0' is the first frame, and frame '-0' is the last frame)
	if (!this->curFrame && signbit(this->playSpeed) && !signbit(this->curFrame))
		this->curFrame *= -1;
	
	float curFrame = this->curFrame;
	
	n64_segment_set(obj->segment, anim->object->file->data);
	
	this->endFrame = anim->numFrames - 1;
	
	// animation in reverse
	// TODO consolidate these so there's less code
	if (signbit(curFrame))
	{
		curFrame = this->endFrame + fmodf(curFrame, this->endFrame);
		
		SkelAnime_GetFrameData(anim, floor(curFrame), this->limbCount, this->jointTable);
		SkelAnime_GetFrameData(anim, wrapf(floor(curFrame) - 1, 0, this->endFrame), this->limbCount, this->morphTable);
		
		SkelAnime_InterpFrameTable(
			this->limbCount,
			this->jointTable,
			this->jointTable,
			this->morphTable,
			1.0 - fmod(curFrame, 1.0f)
		);
	}
	else
	{
		curFrame = fmodf(curFrame, this->endFrame);
		
		SkelAnime_GetFrameData(anim, floor(curFrame), this->limbCount, this->jointTable);
		SkelAnime_GetFrameData(anim, wrapf(floor(curFrame) + 1, 0, this->endFrame), this->limbCount, this->morphTable);
		
		SkelAnime_InterpFrameTable(
			this->limbCount,
			this->jointTable,
			this->jointTable,
			this->morphTable,
			fmod(curFrame, 1.0f)
		);
	}
	
	this->curFrame += this->playSpeed * deltaTimeFrames;
	
	this->prevFrame = curFrame;
}

static void SkelAnime_Limb(const struct Object *obj, const uint32_t skelSeg, u8 limbId, MtxN64** mtx, Vec3s* jointTable, const struct ObjectLimbOverride *limbOverrides)
{
	const StandardLimb* limb;
	const uint32_t* limbList;
	Vec3s rot = { 0 };
	Vec3s pos;
	Vec3f rpos;
	
	limbList = SEGMENTED_TO_VIRTUAL(skelSeg);
	limb = SEGMENTED_TO_VIRTUAL(u32r(&limbList[limbId]));
	
	Matrix_Push();
	{
		if (limbId == 0)
		{
			pos = jointTable[0];
			rot = jointTable[1];
		}
		else
		{
			pos = (Vec3s){ u16r3(&limb->jointPos) };
			
			limbId++;
			rot = jointTable[limbId];
		}
		
		veccpy(&rpos, &pos);
		//LogDebug("limb[%d] pos = %d %d %d", limbId, pos.x, pos.y, pos.z);
		//static Vec3f rpos; float speed = 0.01; rpos.x += speed; rpos.y += speed; rpos.z += speed;
		
		Matrix_TranslateRotateZYX(&rpos, &rot);
		
		const struct ObjectLimbOverride *override = FindLimbOverride(limbOverrides, limbId);
		if (limb->dList || override)
		{
			if (mtx && *mtx)
			{
				Matrix_ToMtx((*mtx));
				gSPMatrix(POLY_OPA_DISP++, (*mtx), G_MTX_LOAD);
				(*mtx)++;
			}
			
			bool restoreObject = false;
			uint32_t dlist = u32r(&limb->dList);
			//if (limbOverrides) LogDebug("limbOverrides = %p, %d = %08x vs %d vs %p", limbOverrides, limbOverrides[0].limbIndex, limbOverrides[0].segAddr, limbId, override);
			if (override)
			{
				dlist = override->segAddr;//, LogDebug("override %d with %08x", limbId, override->segAddr);
				
				if (override->objectData)
				{
					gSPSegment(POLY_OPA_DISP++, dlist >> 24, override->objectData);
					restoreObject = true;
				}
			}
			
			gSPDisplayList(POLY_OPA_DISP++, dlist);
			
			if (restoreObject)
				gSPSegment(POLY_OPA_DISP++, obj->segment, obj->file->data);
		}
		
		if (limb->child != 0xFF)
			SkelAnime_Limb(obj, skelSeg, limb->child, mtx, jointTable, limbOverrides);
	}
	Matrix_Pop();
	
	if (limb->sibling != 0xFF)
		SkelAnime_Limb(obj, skelSeg, limb->sibling, mtx, jointTable, limbOverrides);
}

void SkelAnime_Draw(SkelAnime* this, SkelanimeType type, const struct ObjectLimbOverride *limbOverrides)
{
	const struct Object *obj = this->object;
	const struct ObjectSkeleton *skel = this->skeleton;
	MtxN64* mtx = NULL;
	
	n64_segment_set(obj->segment, obj->file->data);
	gSPSegment(POLY_OPA_DISP++, obj->segment, obj->file->data);
	
	Matrix_Push();
	{
		if (type == SKELANIME_TYPE_FLEX)
		{
			mtx = n64_graph_alloc(sizeof(MtxN64) * skel->limbCount);
			
			gSPSegment(POLY_OPA_DISP++, 0x0D, mtx);
		}
		
		SkelAnime_Limb(obj, skel->limbAddrsSegAddr, 0, &mtx, this->jointTable, limbOverrides);
	}
	Matrix_Pop();
}
