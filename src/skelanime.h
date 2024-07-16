#ifndef SKELANIME_H_INCLUDED
#define SKELANIME_H_INCLUDED

#include "object.h"
#include "extmath.h"

typedef struct
{
	Vec3s jointPos;
	u8    child;
	u8    sibling;
	u32   dList;
} StandardLimb;

typedef struct SkelAnime
{
	u8    limbCount;
	u8    mode;
	u8    dListCount;
	s8    taper;
	const struct ObjectSkeleton *skeleton;
	const struct ObjectAnimation *animation;
	const struct Object *object;
	f32   startFrame;
	f32   endFrame;
	f32   animLength;
	f32   curFrame;
	f32   playSpeed;
	Vec3s jointTable[100];
	Vec3s morphTable[100];
	f32   morphWeight;
	f32   morphRate;
	s32 (* update)();
	s8    initFlags;
	u8    moveFlags;
	s16   prevRot;
	Vec3s prevTransl;
	Vec3s baseTransl;
	f32   prevFrame;
	f64 time;
} SkelAnime;

typedef enum
{
	SKELANIME_TYPE_BASIC,
	SKELANIME_TYPE_FLEX,
} SkelanimeType;

void SkelAnime_Init(SkelAnime*, const struct Object *object, const struct ObjectSkeleton *skeleton, const struct ObjectAnimation *animation);
void SkelAnime_Update(SkelAnime*, double deltaTimeFrames);
void SkelAnime_Draw(SkelAnime*, SkelanimeType type, const struct ObjectLimbOverride *limbOverrides);
void SkelAnime_Free(SkelAnime*);

#endif // SKELANIME_H_INCLUDED
