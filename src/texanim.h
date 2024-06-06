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

#if 1 // region: types

typedef uint32_t TexturePtr; // segment address

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
	/* 0x0 */ uint16_t keyFrameLength;
	/* 0x2 */ uint16_t keyFrameCount;
	/* 0x4 */ F3DPrimColor* primColors;
	/* 0x8 */ F3DEnvColor* envColors;
	/* 0xC */ uint16_t* keyFrames;
} AnimatedMatColorParams; // size = 0x10

typedef struct {
	/* 0x0 */ int8_t xStep;
	/* 0x1 */ int8_t yStep;
	/* 0x2 */ uint8_t width;
	/* 0x3 */ uint8_t height;
} AnimatedMatTexScrollParams; // size = 0x4

typedef struct {
	/* 0x0 */ uint16_t keyFrameLength;
	/* 0x4 */ TexturePtr* textureList; // segmet addresses
	/* 0x8 */ uint8_t* textureIndexList;
} AnimatedMatTexCycleParams; // size = 0xC

typedef struct {
	/* 0x0 */ int8_t segment;
	/* 0x2 */ int16_t type;
	/* 0x4 */ void* params;
} AnimatedMaterial; // size = 0x8

#endif // endregion

#if 1 // region: function prototypes

GbiGfx* Gfx_TexScroll(uint32_t x, uint32_t y, int32_t width, int32_t height);
GbiGfx* Gfx_TwoTexScroll(int32_t tile1, uint32_t x1, uint32_t y1, int32_t width1, int32_t height1, int32_t tile2, uint32_t x2, uint32_t y2, int32_t width2, int32_t height2);

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
void AnimatedMat_DrawStep(AnimatedMaterial* matAnim, uint32_t step);

// Draws an animated material with a step to only the OPA buffer.
void AnimatedMat_DrawStepOpa(AnimatedMaterial* matAnim, uint32_t step);

// Draws an animated material with a step to only the XLU buffer.
void AnimatedMat_DrawStepXlu(AnimatedMaterial* matAnim, uint32_t step);

// Draws an animated material with an alpha ratio (0.0 - 1.0) and a step to both the OPA and XLU buffers.
void AnimatedMat_DrawAlphaStep(AnimatedMaterial* matAnim, float alphaRatio, uint32_t step);

// Draws an animated material with an alpha ratio (0.0 - 1.0) and a step to only the OPA buffer.
void AnimatedMat_DrawAlphaStepOpa(AnimatedMaterial* matAnim, float alphaRatio, uint32_t step);

// Draws an animated material with an alpha ratio (0.0 - 1.0) and a step to only the XLU buffer.
void AnimatedMat_DrawAlphaStepXlu(AnimatedMaterial* matAnim, float alphaRatio, uint32_t step);

#endif // endregion

#endif // TEXANIM_H_INCLUDED
