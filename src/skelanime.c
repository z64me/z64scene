
#include "misc.h"
#include "skelanime.h"
#include "object.h"

#include <string.h>
#include <stdio.h>

#include <n64.h>

#define SEGMENTED_TO_VIRTUAL(X) n64_segment_get(X)

void SkelAnime_Init(SkelAnime *this, const struct Object *object, const struct ObjectSkeleton *skeleton, const struct ObjectAnimation *animation)
{
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
	const struct Object *obj = this->object;
	
	n64_segment_set(obj->segment, obj->file->data);
	
	this->endFrame = anim->numFrames - 1;
	SkelAnime_GetFrameData(anim, floor(this->curFrame), this->limbCount, this->jointTable);
	SkelAnime_GetFrameData(anim, wrapf(floor(this->curFrame) + 1, 0, this->endFrame), this->limbCount, this->morphTable);
	
	SkelAnime_InterpFrameTable(
		this->limbCount,
		this->jointTable,
		this->jointTable,
		this->morphTable,
		fmod(this->curFrame, 1.0f)
	);
	
	if (this->curFrame < this->endFrame)
		this->curFrame += this->playSpeed * deltaTimeFrames;
	else
		this->curFrame = 0;
	
	this->prevFrame = this->curFrame;
}

static void SkelAnime_Limb(const uint32_t skelSeg, u8 limbId, MtxN64** mtx, Vec3s* jointTable)
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
		
		if (limb->dList)
		{
			if (mtx && *mtx)
			{
				Matrix_ToMtx((*mtx));
				gSPMatrix(POLY_OPA_DISP++, (*mtx), G_MTX_LOAD);
				(*mtx)++;
			}
			
			gSPDisplayList(POLY_OPA_DISP++, u32r(&limb->dList));
		}
		
		if (limb->child != 0xFF)
			SkelAnime_Limb(skelSeg, limb->child, mtx, jointTable);
	}
	Matrix_Pop();
	
	if (limb->sibling != 0xFF)
		SkelAnime_Limb(skelSeg, limb->sibling, mtx, jointTable);
}

void SkelAnime_Draw(SkelAnime* this, SkelanimeType type)
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
		
		SkelAnime_Limb(skel->limbAddrsSegAddr, 0, &mtx, this->jointTable);
	}
	Matrix_Pop();
}
