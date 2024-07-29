//
// texanim.h
//
// texture animation code lives here
//

#ifndef TEXANIM_H_INCLUDED
#define TEXANIM_H_INCLUDED

#ifdef __cplusplus
typedef void Gfx;
typedef void GbiGfx;
#else
#include <n64.h>
#define Gfx GbiGfx
#endif

typedef float texAnimStep_t;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;

#include "stretchy_buffer.h"

#if 1 // region: types

#define USE_TEXANIM_MM_LOOP_HACK(STRUCTPTR) \
	(STRUCTPTR)->type != AnimatedMatType_DrawColor

typedef enum AnimatedMatType
{
	AnimatedMatType_DrawTexScroll,
	AnimatedMatType_DrawTwoTexScroll,
	AnimatedMatType_DrawColor,
	AnimatedMatType_DrawColorLerp,
	AnimatedMatType_DrawColorNonLinearInterp,
	AnimatedMatType_DrawTexCycle,
	AnimatedMatType_DoNothing,
	// extended functionality
	AnimatedMatType_SceneAnim_Pointer_Flag,
	AnimatedMatType_SceneAnim_TexScroll_Flag,
	AnimatedMatType_SceneAnim_Color_Loop,
	AnimatedMatType_SceneAnim_Color_LoopFlag,
	AnimatedMatType_SceneAnim_Pointer_Loop,
	AnimatedMatType_SceneAnim_Pointer_LoopFlag,
	AnimatedMatType_SceneAnim_Pointer_Timeloop,
	AnimatedMatType_SceneAnim_Pointer_TimeloopFlag,
	AnimatedMatType_SceneAnim_CameraEffect,
	AnimatedMatType_SceneAnim_DrawCondition,
	AnimatedMatType_Count,
} AnimatedMatType;

#define ANIMATED_MAT_SEGMENT_OFFSET 7

typedef struct {
	uint32_t addr;
	uint32_t addrBEU32;
	void *datablob;
} TexturePtr; // segment address

typedef struct {
	/* 0x0 */ uint8_t r;
	/* 0x1 */ uint8_t g;
	/* 0x2 */ uint8_t b;
	/* 0x3 */ uint8_t a;
	/* 0x4 */ uint8_t lodFrac;
} F3DPrimColor; // size = 0x5

typedef struct {
	/* 0x0 */ uint8_t r;
	/* 0x1 */ uint8_t g;
	/* 0x2 */ uint8_t b;
	/* 0x3 */ uint8_t a;
} F3DEnvColor; // size = 0x4

typedef struct {
	/* 0x0 */ uint16_t durationFrames;
	/* 0x2 */ uint16_t keyFrameCount;
	/* 0x4 */ sb_array(F3DPrimColor, primColors);
	/* 0x8 */ sb_array(F3DEnvColor, envColors);
	/* 0xC */ sb_array(uint16_t, keyFrames);
	sb_array(uint16_t, durationEachKey);
} AnimatedMatColorParams; // size = 0x10

typedef struct {
	/* 0x0 */ int8_t xStep;
	/* 0x1 */ int8_t yStep;
	/* 0x2 */ uint8_t width;
	/* 0x3 */ uint8_t height;
} AnimatedMatTexScrollParams; // size = 0x4

typedef struct {
	/* 0x0 */ uint16_t durationFrames;
	/* 0x4 */ sb_array(TexturePtr, textureList); // segmet addresses
	/* 0x8 */ sb_array(uint8_t, textureIndexList);
} AnimatedMatTexCycleParams; // size = 0xC

#if 1 // region: extended functionality

typedef enum
{
	SCENE_ANIM_FLAG_TYPE_ROOMCLEAR = 0,
	SCENE_ANIM_FLAG_TYPE_TREASURE,
	SCENE_ANIM_FLAG_TYPE_USCENE,
	SCENE_ANIM_FLAG_TYPE_TEMP,
	SCENE_ANIM_FLAG_TYPE_SCENECOLLECT,
	SCENE_ANIM_FLAG_TYPE_SWITCH,
	SCENE_ANIM_FLAG_TYPE_EVENTCHKINF,
	SCENE_ANIM_FLAG_TYPE_INFTABLE,
	SCENE_ANIM_FLAG_TYPE_IS_NIGHT,
	
	// NOTE: for the following types, flag must be 4-byte-aligned
	
	SCENE_ANIM_FLAG_TYPE_SAVE,    // flag = word(save_ctx[flag]) & bits
	SCENE_ANIM_FLAG_TYPE_GLOBAL,  // flag = word(global_ctx[flag]) & bits
	SCENE_ANIM_FLAG_TYPE_RAM      // flag = word(flag) & bits
} SceneAnimFlagType;

typedef struct
{
	u32 flag;                   // flag or offset
	u32 bits;                   // bit selection
	u8  type;                   // flag type (SceneAnimFlagType)
	u8  eq;                     // if (flag() == eq)
	u16 xfade;                  // crossfade (color)
	u16 freeze;                 // tells if command should be freeze or not written
	u16 frames;                 // frames flag is on
} SceneAnimFlag;

// data processed by SceneAnim_Pointer_Flag functions
typedef struct
{
	u32 ptr[2];                 // pointer pointers
	SceneAnimFlag flag;         // flag structure
	u32 ptrBEU32[2];
} SceneAnimPointerFlag;

// data processed by SceneAnim_TexScroll_One functions
typedef struct
{
	s8 u;                       // u speed
	s8 v;                       // v speed
	u8 w;                       // texture w
	u8 h;                       // texture h
} SceneAnimTexScroll;

// data processed by SceneAnim_TexScroll_Flag functions
typedef struct
{
	SceneAnimTexScroll sc[2];   // SceneAnim_TexScroll_One contents
	SceneAnimFlag      flag;
} SceneAnimTexScrollFlag;

typedef enum
{
	SCENE_ANIM_COLORKEY_PRIM     = 1 << 0,
	SCENE_ANIM_COLORKEY_ENV      = 1 << 1,
	SCENE_ANIM_COLORKEY_LODFRAC  = 1 << 2,
	SCENE_ANIM_COLORKEY_MINLEVEL = 1 << 3,
} SceneAnimColorKeyTypes;

typedef enum
{
	SCENE_ANIM_EASE_LINEAR = 0,
	SCENE_ANIM_EASE_SIN_IN,
	SCENE_ANIM_EASE_SIN_OUT,
} SceneAnimEaseMethod;

// substructure used to describe color keyframe
typedef struct
{
	u32 prim;                 // primcolor (rgba)
	u32 env;                  // envcolor  (rgba)
	u8  lfrac;                // lodfrac   (prim)
	u8  mlevel;               // minlevel  (prim)
	u16 next;                 // frames til next; 0 = last frame
} SceneAnimColorKey;

// data processed by color functions
typedef struct
{
	u8   which;                // units to compute (SceneAnimColorKey)
	u8   ease;                 // ease function (SceneAnimEaseMethod)
	u16  dur;                  // duration
	sb_array(SceneAnimColorKey, key);  // keyframe storage
} SceneAnimColorList;

typedef struct
{
	SceneAnimFlag      flag;   // flag structure
	SceneAnimColorList list;   // color structure
} SceneAnimColorListFlag;

typedef struct
{
	u16 dur;                   // duration of full cycle (frames)
	u16 time;                  // frames elapsed (internal use)
	u16 each;                  // frames to display each item
	u16 pad;                   // unused; padding
	sb_array(u32, ptr);        // list: (dur/each) elements long
	sb_array(u32, ptrBEU32);   // same as above, but encoded for export
} SceneAnimPointerLoop;

typedef struct
{
	SceneAnimFlag flag;        // flag structure
	SceneAnimPointerLoop list; // list structure
} SceneAnimPointerLoopFlag;

// each frame can have its own time
typedef struct
{
	u16 prev;                  // item selected (previous frame)
	u16 time;                  // frames elapsed (internal use)
	u16 num;                   // number of pointers in list
	sb_array(u16, each);       // first frame of each pointer
	sb_array(u32, ptr);        // see notes on alignment and size
	
	/* NOTE: each[1] can be any length; now imagine immediately   *
	*        after it, there is a ptr[1], containing one pointer  *
	*        for each item; ptr[1] must be aligned by 4 bytes;    *
	*        so to get a pointer to it, do the following:         *
	*        u32 *ptr = (void*)(each + num + !(num & 1));         *
	*        see the functions that process this structure if     *
	*        you're having trouble                                */
	
	/* NOTE: each[] contains num items (the last indicating the   *
	*        end frame), and ptr[] contains num-1 items           */
} SceneAnimPointerTimeloop;

typedef struct
{
	SceneAnimFlag flag;             // flag structure
	SceneAnimPointerTimeloop list;  // list structure
} SceneAnimPointerTimeloopFlag;

typedef struct
{
	SceneAnimFlag flag;       // flag structure
	u8 cameraType;            // camera type
	u8 set;                   // used ingame to tell that the camera is set
} SceneAnimCameraEffect;

typedef struct
{
	SceneAnimFlag flag;       // flag structure
} SceneAnimDrawCondition;

#endif // end region: extended functionality

typedef struct AnimatedMaterial
{
	/* 0x0 */ int8_t segment;
	/* 0x2 */ enum AnimatedMatType type;
	/* 0x4 */ sb_array(void, params);
	void *datablob; // datablob associated with segment
} AnimatedMaterial; // size = 0x8

#endif // endregion

#if 1 // region: function prototypes

const char **AnimatedMatType_Names(void);
const char *AnimatedMatType_AsString(enum AnimatedMatType type);
GbiGfx* Gfx_TexScroll(uint32_t x, uint32_t y, int32_t width, int32_t height);
GbiGfx* Gfx_TwoTexScroll(int32_t tile1, uint32_t x1, uint32_t y1, int32_t width1, int32_t height1, int32_t tile2, uint32_t x2, uint32_t y2, int32_t width2, int32_t height2);
AnimatedMaterial *AnimatedMaterialNewFromSegment(uint32_t segAddr);
AnimatedMaterial AnimatedMaterialNewFromDefault(AnimatedMatType type, int segment);
void AnimatedMaterialFreeList(AnimatedMaterial *sbArr);
void AnimatedMaterialFree(AnimatedMaterial *mat);
void TexAnimSetGameplayFrames(float frames);

void AnimatedMaterialToWorkblob(
	AnimatedMaterial *matAnim
	, void WorkblobPush(uint8_t alignBytes)
	, uint32_t WorkblobPop(void)
	, void WorkblobPut8(uint8_t data)
	, void WorkblobPut16(uint16_t data)
	, void WorkblobPut32(uint32_t data)
);

// Executes the current scene draw config handler.
void TexAnimSetupSceneMM(int which, AnimatedMaterial *sceneMaterialAnims);

// Draws an animated material to both OPA and XLU buffers.
void AnimatedMat_Draw(AnimatedMaterial* matAnim);

// Draws an animated material to only the OPA buffer.
void AnimatedMat_DrawOpa(AnimatedMaterial* matAnim);

// Draws an animated material to only the XLU buffer.
void AnimatedMat_DrawXlu(AnimatedMaterial* matAnim);

// Draws an animated material with an alpha ratio (0.0 - 1.0) both OPA and XLU buffers.
void AnimatedMat_DrawAlpha(AnimatedMaterial* matAnim, float alphaRatio);

// Draws an animated material with an alpha ratio (0.0 - 1.0) to only the OPA buffer.
void AnimatedMat_DrawAlphaOpa(AnimatedMaterial* matAnim, float alphaRatio);

// Draws an animated material with an alpha ratio (0.0 - 1.0) to only the XLU buffer.
void AnimatedMat_DrawAlphaXlu(AnimatedMaterial* matAnim, float alphaRatio);

// Draws an animated material with a step to both the OPA and XLU buffers.
void AnimatedMat_DrawStep(AnimatedMaterial* matAnim, texAnimStep_t step);

// Draws an animated material with a step to only the OPA buffer.
void AnimatedMat_DrawStepOpa(AnimatedMaterial* matAnim, texAnimStep_t step);

// Draws an animated material with a step to only the XLU buffer.
void AnimatedMat_DrawStepXlu(AnimatedMaterial* matAnim, texAnimStep_t step);

// Draws an animated material with an alpha ratio (0.0 - 1.0) and a step to both the OPA and XLU buffers.
void AnimatedMat_DrawAlphaStep(AnimatedMaterial* matAnim, float alphaRatio, texAnimStep_t step);

// Draws an animated material with an alpha ratio (0.0 - 1.0) and a step to only the OPA buffer.
void AnimatedMat_DrawAlphaStepOpa(AnimatedMaterial* matAnim, float alphaRatio, texAnimStep_t step);

// Draws an animated material with an alpha ratio (0.0 - 1.0) and a step to only the XLU buffer.
void AnimatedMat_DrawAlphaStepXlu(AnimatedMaterial* matAnim, float alphaRatio, texAnimStep_t step);

#endif // endregion

#endif // TEXANIM_H_INCLUDED
