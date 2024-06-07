//
// texanim.c
//
// texture animation code lives here
//

#include "texanim.h"
#include "misc.h"

#include <n64.h>
#include <stdio.h>

#define GRAPH_ALLOC n64_graph_alloc
#define Gfx GbiGfx
#define OPEN_DISPS() do{}while(0)
#define CLOSE_DISPS() do{}while(0)
#define Lib_SegmentedToVirtual(X) X // segmented addresses are resolved on load, so this isn't necessary

#undef ABS
#define ABS(x) ((x) >= 0 ? (x) : -(x))
#define ABS_ALT(x) ((x) < 0 ? -(x) : (x))

//! checks min first
#define CLAMP(x, min, max) ((x) < (min) ? (min) : (x) > (max) ? (max) : (x))
//! checks max first
#define CLAMP_ALT(x, min, max) ((x) > (max) ? (max) : (x) < (min) ? (min) : (x))
#define CLAMP_MAX(x, max) ((x) > (max) ? (max) : (x))
#define CLAMP_MIN(x, min) ((x) < (min) ? (min) : (x))

static texAnimStep_t sGameplayFrames;

#if 1 // region: private types

typedef struct {
	/* 0x0 */ uint16_t durationFrames;
	/* 0x2 */ uint16_t keyFrameCount;
	/* 0x4 */ uint32_t primColors;
	/* 0x8 */ uint32_t envColors;
	/* 0xC */ uint32_t keyFrames;
} AnimatedMatColorParams64; // size = 0x10

typedef struct {
	/* 0x0 */ int8_t xStep;
	/* 0x1 */ int8_t yStep;
	/* 0x2 */ uint8_t width;
	/* 0x3 */ uint8_t height;
} AnimatedMatTexScrollParams64; // size = 0x4

typedef struct {
	/* 0x0 */ uint16_t durationFrames;
	/* 0x4 */ uint32_t textureList; // segmet addresses
	/* 0x8 */ uint32_t textureIndexList;
} AnimatedMatTexCycleParams64; // size = 0xC

typedef struct {
	/* 0x0 */ int8_t segment;
	/* 0x2 */ int16_t type;
	/* 0x4 */ uint32_t params;
} AnimatedMaterial64; // size = 0x8

#endif // endregion

#if 1 // region: mm, animatedmat, private

static texAnimStep_t sMatAnimStep;
static uint32_t sMatAnimFlags;
static float sMatAnimAlphaRatio;
static AnimatedMaterial *sSceneMaterialAnims;

/**
 * Returns a pointer to a single layer texture scroll displaylist.
 */
static Gfx* AnimatedMat_TexScroll(AnimatedMatTexScrollParams* params) {
	return Gfx_TexScroll(params->xStep * sMatAnimStep, -(params->yStep * sMatAnimStep),
		params->width, params->height
	);
}

/**
 * Animated Material Type 0:
 * Scrolls a single layer texture using the provided `AnimatedMatTexScrollParams`.
 */
static void AnimatedMat_DrawTexScroll(int32_t segment, void* params) {
	AnimatedMatTexScrollParams* texScrollParams = (AnimatedMatTexScrollParams*)params;
	Gfx* texScrollDList = AnimatedMat_TexScroll(texScrollParams);
	
	OPEN_DISPS();
	
	if (sMatAnimFlags & 1) {
		gSPSegment(POLY_OPA_DISP++, segment, texScrollDList);
	}
	
	if (sMatAnimFlags & 2) {
		gSPSegment(POLY_XLU_DISP++, segment, texScrollDList);
	}
	
	CLOSE_DISPS();
}

/**
 * Returns a pointer to a two layer texture scroll displaylist.
 */
static Gfx* AnimatedMat_TwoLayerTexScroll(AnimatedMatTexScrollParams* params) {
	return Gfx_TwoTexScroll(0, params[0].xStep * sMatAnimStep, -(params[0].yStep * sMatAnimStep),
		params[0].width, params[0].height, 1, params[1].xStep * sMatAnimStep,
		-(params[1].yStep * sMatAnimStep), params[1].width, params[1].height
	);
}

/**
 * Animated Material Type 1:
 * Scrolls a two layer texture using the provided `AnimatedMatTexScrollParams`.
 */
static void AnimatedMat_DrawTwoTexScroll(int32_t segment, void* params) {
	AnimatedMatTexScrollParams* texScrollParams = (AnimatedMatTexScrollParams*)params;
	Gfx* texScrollDList = AnimatedMat_TwoLayerTexScroll(texScrollParams);
	
	OPEN_DISPS();
	
	if (sMatAnimFlags & 1) {
		gSPSegment(POLY_OPA_DISP++, segment, texScrollDList);
	}
	
	if (sMatAnimFlags & 2) {
		gSPSegment(POLY_XLU_DISP++, segment, texScrollDList);
	}
	
	CLOSE_DISPS();
}

/**
 * Generates a displaylist that sets the prim and env color, and stores it in the provided segment ID.
 */
static void AnimatedMat_SetColor(int32_t segment, F3DPrimColor* primColorResult, F3DEnvColor* envColor) {
	Gfx* gfx = GRAPH_ALLOC(3 * sizeof(Gfx));
	
	OPEN_DISPS();
	
	// clang-format off
	if (sMatAnimFlags & 1) { gSPSegment(POLY_OPA_DISP++, segment, gfx); }
	if (sMatAnimFlags & 2) { gSPSegment(POLY_XLU_DISP++, segment, gfx); }
	// clang-format on
	
	gDPSetPrimColor(gfx++, 0, primColorResult->lodFrac, primColorResult->r, primColorResult->g, primColorResult->b,
		(uint8_t)(primColorResult->a * sMatAnimAlphaRatio)
	);
	
	if (envColor != NULL) {
		gDPSetEnvColor(gfx++, envColor->r, envColor->g, envColor->b, envColor->a);
	}
	
	gSPEndDisplayList(gfx++);
	
	CLOSE_DISPS();
}

/**
 * Animated Material Type 2:
 * Color key frame animation without linear interpolation.
 */
static void AnimatedMat_DrawColor(int32_t segment, void* params) {
	AnimatedMatColorParams* colorAnimParams = (AnimatedMatColorParams*)params;
	F3DPrimColor* primColor = Lib_SegmentedToVirtual(colorAnimParams->primColors);
	F3DEnvColor* envColor;
	int32_t curFrame = (uint32_t)sMatAnimStep % colorAnimParams->durationFrames;
	
	primColor += curFrame;
	envColor = (colorAnimParams->envColors != NULL)
		? (F3DEnvColor*)Lib_SegmentedToVirtual(colorAnimParams->envColors) + curFrame
		: NULL
	;
	
	AnimatedMat_SetColor(segment, primColor, envColor);
}

/**
 * Linear Interpolation
 */
static int32_t AnimatedMat_Lerp(int32_t min, int32_t max, float norm) {
	return (int32_t)((max - min) * norm) + min;
}

/**
 * Animated Material Type 3:
 * Color key frame animation with linear interpolation.
 */
static void AnimatedMat_DrawColorLerp(int32_t segment, void* params) {
	AnimatedMatColorParams* colorAnimParams = (AnimatedMatColorParams*)params;
	F3DPrimColor* primColorMax = Lib_SegmentedToVirtual(colorAnimParams->primColors);
	F3DEnvColor* envColorMax;
	uint16_t* keyFrames = Lib_SegmentedToVirtual(colorAnimParams->keyFrames);
	int32_t curFrame = (uint32_t)sMatAnimStep % colorAnimParams->durationFrames;
	int32_t endFrame;
	int32_t relativeFrame; // relative to the start frame
	int32_t startFrame;
	float norm;
	F3DPrimColor* primColorMin;
	F3DPrimColor primColorResult;
	F3DEnvColor* envColorMin;
	F3DEnvColor envColorResult;
	int32_t i;
	
	keyFrames++;
	i = 1;
	
	while (colorAnimParams->keyFrameCount > i) {
		if (curFrame < *keyFrames) {
			break;
		}
		i++;
		keyFrames++;
	}
	
	startFrame = keyFrames[-1];
	endFrame = keyFrames[0] - startFrame;
	relativeFrame = curFrame - startFrame;
	norm = (float)relativeFrame / (float)endFrame;
	
	primColorMax += i;
	primColorMin = primColorMax - 1;
	primColorResult.r = AnimatedMat_Lerp(primColorMin->r, primColorMax->r, norm);
	primColorResult.g = AnimatedMat_Lerp(primColorMin->g, primColorMax->g, norm);
	primColorResult.b = AnimatedMat_Lerp(primColorMin->b, primColorMax->b, norm);
	primColorResult.a = AnimatedMat_Lerp(primColorMin->a, primColorMax->a, norm);
	primColorResult.lodFrac = AnimatedMat_Lerp(primColorMin->lodFrac, primColorMax->lodFrac, norm);
	
	if (colorAnimParams->envColors) {
		envColorMax = Lib_SegmentedToVirtual(colorAnimParams->envColors);
		envColorMax += i;
		envColorMin = envColorMax - 1;
		envColorResult.r = AnimatedMat_Lerp(envColorMin->r, envColorMax->r, norm);
		envColorResult.g = AnimatedMat_Lerp(envColorMin->g, envColorMax->g, norm);
		envColorResult.b = AnimatedMat_Lerp(envColorMin->b, envColorMax->b, norm);
		envColorResult.a = AnimatedMat_Lerp(envColorMin->a, envColorMax->a, norm);
	} else {
		envColorMax = NULL;
	}
	
	AnimatedMat_SetColor(segment, &primColorResult, (envColorMax != NULL) ? &envColorResult : NULL);
}

/**
 * Lagrange interpolation
 */
static float Scene_LagrangeInterp(int32_t n, float x[], float fx[], float xp) {
	float weights[50];
	float xVal;
	float m;
	float intp;
	float* xPtr1;
	float* fxPtr;
	float* weightsPtr;
	float* xPtr2;
	int32_t i;
	int32_t j;
	
	for (i = 0, xPtr1 = x, fxPtr = fx, weightsPtr = weights; i < n; i++) {
		for (xVal = *xPtr1, m = 1.0f, j = 0, xPtr2 = x; j < n; j++) {
			if (j != i) {
				m *= xVal - (*xPtr2);
			}
			xPtr2++;
		}
		
		xPtr1++;
		*weightsPtr = (*fxPtr) / m;
		fxPtr++;
		weightsPtr++;
	}
	
	for (intp = 0.0f, i = 0, weightsPtr = weights; i < n; i++) {
		for (m = 1.0f, j = 0, xPtr2 = x; j < n; j++) {
			if (j != i) {
				m *= xp - (*xPtr2);
			}
			xPtr2++;
		}
		
		intp += (*weightsPtr) * m;
		weightsPtr++;
	}
	
	return intp;
}

/**
 * Lagrange interpolation specifically for colors
 */
static uint8_t Scene_LagrangeInterpColor(int32_t n, float x[], float fx[], float xp) {
	int32_t intp = Scene_LagrangeInterp(n, x, fx, xp);
	
	// Clamp between 0 and 255 to ensure the color value does not overflow in either direction
	return CLAMP(intp, 0, 255);
}

/**
 * Animated Material Type 4:
 * Color key frame animation with non-linear interpolation.
 */
static void AnimatedMat_DrawColorNonLinearInterp(int32_t segment, void* params) {
	AnimatedMatColorParams* colorAnimParams = (AnimatedMatColorParams*)params;
	F3DPrimColor* primColorCur = Lib_SegmentedToVirtual(colorAnimParams->primColors);
	F3DEnvColor* envColorCur = Lib_SegmentedToVirtual(colorAnimParams->envColors);
	uint16_t* keyFrames = Lib_SegmentedToVirtual(colorAnimParams->keyFrames);
	float curFrame = (uint32_t)sMatAnimStep % colorAnimParams->durationFrames;
	F3DPrimColor primColorResult;
	F3DEnvColor envColorResult;
	float x[50];
	float fxPrimR[50];
	float fxPrimG[50];
	float fxPrimB[50];
	float fxPrimA[50];
	float fxPrimLodFrac[50];
	float fxEnvR[50];
	float fxEnvG[50];
	float fxEnvB[50];
	float fxEnvA[50];
	float* xPtr = x;
	float* fxPrimRPtr = fxPrimR;
	float* fxPrimGPtr = fxPrimG;
	float* fxPrimBPtr = fxPrimB;
	float* fxPrimAPtr = fxPrimA;
	float* fxPrimLodFracPtr = fxPrimLodFrac;
	float* fxEnvRPtr = fxEnvR;
	float* fxEnvGPtr = fxEnvG;
	float* fxEnvBPtr = fxEnvB;
	float* fxEnvAPtr = fxEnvA;
	int32_t i;
	
	for (i = 0; i < colorAnimParams->keyFrameCount; i++) {
		*xPtr = *keyFrames;
		*fxPrimRPtr = primColorCur->r;
		*fxPrimGPtr = primColorCur->g;
		*fxPrimBPtr = primColorCur->b;
		*fxPrimAPtr = primColorCur->a;
		*fxPrimLodFracPtr = primColorCur->lodFrac;
		
		primColorCur++;
		fxPrimRPtr++;
		fxPrimGPtr++;
		fxPrimBPtr++;
		fxPrimAPtr++;
		fxPrimLodFracPtr++;
		
		if (envColorCur != NULL) {
			*fxEnvRPtr = envColorCur->r;
			*fxEnvGPtr = envColorCur->g;
			*fxEnvBPtr = envColorCur->b;
			*fxEnvAPtr = envColorCur->a;

			envColorCur++;
			fxEnvRPtr++;
			fxEnvGPtr++;
			fxEnvBPtr++;
			fxEnvAPtr++;
		}
		
		keyFrames++;
		xPtr++;
	}
	
	primColorResult.r = Scene_LagrangeInterpColor(colorAnimParams->keyFrameCount, x, fxPrimR, curFrame);
	primColorResult.g = Scene_LagrangeInterpColor(colorAnimParams->keyFrameCount, x, fxPrimG, curFrame);
	primColorResult.b = Scene_LagrangeInterpColor(colorAnimParams->keyFrameCount, x, fxPrimB, curFrame);
	primColorResult.a = Scene_LagrangeInterpColor(colorAnimParams->keyFrameCount, x, fxPrimA, curFrame);
	primColorResult.lodFrac = Scene_LagrangeInterpColor(colorAnimParams->keyFrameCount, x, fxPrimLodFrac, curFrame);
	
	if (colorAnimParams->envColors != NULL) {
		envColorCur = Lib_SegmentedToVirtual(colorAnimParams->envColors);
		envColorResult.r = Scene_LagrangeInterpColor(colorAnimParams->keyFrameCount, x, fxEnvR, curFrame);
		envColorResult.g = Scene_LagrangeInterpColor(colorAnimParams->keyFrameCount, x, fxEnvG, curFrame);
		envColorResult.b = Scene_LagrangeInterpColor(colorAnimParams->keyFrameCount, x, fxEnvB, curFrame);
		envColorResult.a = Scene_LagrangeInterpColor(colorAnimParams->keyFrameCount, x, fxEnvA, curFrame);
	} else {
		envColorCur = NULL;
	}
	
	AnimatedMat_SetColor(segment, &primColorResult, (envColorCur != NULL) ? &envColorResult : NULL);
}

/**
 * Animated Material Type 5:
 * Cycles between a list of textures (imagine like a GIF)
 */
static void AnimatedMat_DrawTexCycle(int32_t segment, void* params) {
	AnimatedMatTexCycleParams* texAnimParams = params;
	TexturePtr* texList = Lib_SegmentedToVirtual(texAnimParams->textureList);
	uint8_t* texId = Lib_SegmentedToVirtual(texAnimParams->textureIndexList);
	int32_t curFrame = (uint32_t)sMatAnimStep % texAnimParams->durationFrames;
	void* tex = n64_segment_get(texList[texId[curFrame]]);
	
	OPEN_DISPS();
	
	if (sMatAnimFlags & 1) {
		gSPSegment(POLY_OPA_DISP++, segment, tex);
	}
	
	if (sMatAnimFlags & 2) {
		gSPSegment(POLY_XLU_DISP++, segment, tex);
	}
	
	CLOSE_DISPS();
}

/**
 * This is the main function that handles the animated material system.
 * There are six different animated material types, which should be set in the provided `AnimatedMaterial`.
 */
static void AnimatedMat_DrawMain(AnimatedMaterial* matAnim, float alphaRatio, texAnimStep_t step, uint32_t flags) {
	static void (*matAnimDrawHandlers[])(int32_t segment, void* params) = {
		AnimatedMat_DrawTexScroll, AnimatedMat_DrawTwoTexScroll, AnimatedMat_DrawColor,
		AnimatedMat_DrawColorLerp, AnimatedMat_DrawColorNonLinearInterp, AnimatedMat_DrawTexCycle,
	};
	int32_t segmentAbs;
	int32_t segment;
	
	sMatAnimAlphaRatio = alphaRatio;
	sMatAnimStep = step;
	sMatAnimFlags = flags;
	
	if ((matAnim != NULL) && (matAnim->segment != 0)) {
		do {
			segment = matAnim->segment;
			segmentAbs = ABS_ALT(segment) + 7;
			//fprintf(stderr, "populate segment 0x%02x type %d params %p\n", segmentAbs, matAnim->type, matAnim->params);
			matAnimDrawHandlers[matAnim->type](segmentAbs, Lib_SegmentedToVirtual(matAnim->params));
			matAnim++;
		} while (segment >= 0);
	}
}

/**
 * Stores a displaylist in the provided segment ID that sets a render mode from the index provided.
 */
static void Scene_SetRenderModeXlu(int32_t index, uint32_t flags) {
	static Gfx renderModeSetNoneDL[] = {
		gsSPEndDisplayList(),
		// These instructions will never get executed
		gsSPEndDisplayList(),
		gsSPEndDisplayList(),
		gsSPEndDisplayList(),
	};
	static Gfx renderModeSetXluSingleCycleDL[] = {
		gsDPSetRenderMode(AA_EN | Z_CMP | IM_RD | CLR_ON_CVG | CVG_DST_WRAP | ZMODE_XLU | FORCE_BL |
			GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1),
			G_RM_AA_ZB_XLU_SURF2
		),
		gsSPEndDisplayList(),
		// These instructions will never get executed
		gsDPSetRenderMode(AA_EN | Z_CMP | IM_RD | CLR_ON_CVG | CVG_DST_WRAP | ZMODE_XLU | FORCE_BL |
			GBL_c1(G_BL_CLR_FOG, G_BL_A_SHADE, G_BL_CLR_IN, G_BL_1MA),
			G_RM_AA_ZB_XLU_SURF2
		),
		gsSPEndDisplayList(),
	};
	static Gfx renderModeSetXluTwoCycleDL[] = {
		gsDPSetRenderMode(AA_EN | Z_CMP | Z_UPD | IM_RD | CLR_ON_CVG | CVG_DST_WRAP | ZMODE_XLU | FORCE_BL |
			GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1),
			AA_EN | Z_CMP | Z_UPD | IM_RD | CLR_ON_CVG | CVG_DST_WRAP | ZMODE_XLU | FORCE_BL |
			GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)
		),
		gsSPEndDisplayList(),
		// These instructions will never get executed
		gsDPSetRenderMode(AA_EN | Z_CMP | Z_UPD | IM_RD | CLR_ON_CVG | CVG_DST_WRAP | ZMODE_XLU | FORCE_BL |
			GBL_c1(G_BL_CLR_FOG, G_BL_A_SHADE, G_BL_CLR_IN, G_BL_1MA),
			AA_EN | Z_CMP | Z_UPD | IM_RD | CLR_ON_CVG | CVG_DST_WRAP | ZMODE_XLU | FORCE_BL |
			GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)
		),
		gsSPEndDisplayList(),
	};
	static Gfx* dLists[] = {
		renderModeSetNoneDL,
		renderModeSetXluSingleCycleDL,
		renderModeSetXluTwoCycleDL,
	};
	Gfx* dList = dLists[index];
	
	OPEN_DISPS();
	
	if (flags & 1) {
		gSPSegment(POLY_OPA_DISP++, 0x0C, dList);
	}
	
	if (flags & 2) {
		gSPSegment(POLY_XLU_DISP++, 0x0C, dList);
	}
	
	CLOSE_DISPS();
}

/**
 * Although this function is unused, it will store a displaylist in the provided segment ID that sets the culling mode
 * from the index provided.
 */
void Scene_SetCullFlag(int32_t index, uint32_t flags) {
	static Gfx setBackCullDL[] = {
		gsSPSetGeometryMode(G_CULL_BACK),
		gsSPEndDisplayList(),
	};
	static Gfx setFrontCullDL[] = {
		gsSPSetGeometryMode(G_CULL_FRONT),
		gsSPEndDisplayList(),
	};
	static Gfx* dLists[] = {
		setBackCullDL,
		setFrontCullDL,
	};
	Gfx* dList = dLists[index];
	
	OPEN_DISPS();
	
	if (flags & 1) {
		gSPSegment(POLY_OPA_DISP++, 0x0C, dList);
	}
	
	if (flags & 2) {
		gSPSegment(POLY_XLU_DISP++, 0x0C, dList);
	}
	
	CLOSE_DISPS();
}

#endif // endregion

#if 1 // region: mm, scenes, private

static uint16_t sPlayRoomCtxUnk78;
static uint8_t sPlayRoomCtxUnk7A[2];

/**
 * SceneTableEntry Draw Config 0:
 * Default scene draw config function. This just executes `sSceneDrawDefaultDL`.
 */
static void DrawConfigDefault(void) {
	static Gfx sEmptyDL[] = {
		gsSPEndDisplayList(),
	};
	// Default displaylist that sets a valid displaylist into all of the segments.
	static Gfx sSceneDrawDefaultDL[] = {
		/*
		gsSPSegment(0x08, sEmptyDL),
		gsSPSegment(0x09, sEmptyDL),
		gsSPSegment(0x0A, sEmptyDL),
		gsSPSegment(0x0B, sEmptyDL),
		gsSPSegment(0x0C, sEmptyDL),
		gsSPSegment(0x0D, sEmptyDL),
		gsSPSegment(0x06, sEmptyDL),
		*/
		gsDPPipeSync(),
		gsDPSetPrimColor(0, 0, 128, 128, 128, 128),
		gsDPSetEnvColor(128, 128, 128, 128),
		gsSPEndDisplayList(),
	};
	OPEN_DISPS();
	
	gSPDisplayList(POLY_OPA_DISP++, sSceneDrawDefaultDL);
	gSPDisplayList(POLY_XLU_DISP++, sSceneDrawDefaultDL);
	
	for (int i = 0x6; i <= 0x0D; ++i)
	{
		if (i != 0x07)
		{
			gSPSegment(POLY_OPA_DISP++, i, sEmptyDL);
			gSPSegment(POLY_XLU_DISP++, i, sEmptyDL);
		}
	}
	
	CLOSE_DISPS();
}

/**
 * SceneTableEntry Draw Config 1:
 * Allows the usage of the animated material system in scenes.
 */
static void DrawConfigMatAnim(void) {
	AnimatedMat_Draw(sSceneMaterialAnims);
}

/**
 * SceneTableEntry Draw Config 3:
 * This config is unused, although it is identical to the grotto scene config from Ocarina of Time.
 */
static void DrawConfig3(void) {
	uint32_t frames;
	
	OPEN_DISPS();
	
	frames = sGameplayFrames;
	
	gSPSegment(POLY_XLU_DISP++, 0x08, Gfx_TexScroll(0, (frames * 1) % 64, 256, 16));
	
	gSPSegment(POLY_XLU_DISP++, 0x09,
		Gfx_TwoTexScroll(0, 127 - (frames % 128), (frames * 1) % 128, 32, 32, 1,
		frames % 128, (frames * 1) % 128, 32, 32)
	);
	
	gSPSegment(POLY_OPA_DISP++, 0x0A,
		Gfx_TwoTexScroll(0, 0, 0, 32, 32, 1, 0, 127 - (frames * 1) % 128, 32, 32)
	);
	
	gSPSegment(POLY_OPA_DISP++, 0x0B, Gfx_TexScroll(0, (frames * 1) % 128, 32, 32));
	
	gSPSegment(
		POLY_XLU_DISP++, 0x0C,
		Gfx_TwoTexScroll(0, 0, (frames * 50) % 2048, 8, 512, 1, 0, (frames * 60) % 2048, 8, 512)
	);
	
	gSPSegment(POLY_OPA_DISP++, 0x0D,
		Gfx_TwoTexScroll(0, 0, 0, 32, 64, 1, 0, (frames * 1) % 128, 32, 32)
	);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	CLOSE_DISPS();
}

/**
 * SceneTableEntry Draw Config 4:
 * This config is unused and just has a single TwoTexScroll intended for two 32x32 textures (likely two water textures).
 * It is identical to the Castle Courtyard and Sacred Forest Meadow scene config from Ocarina of Time.
 */
static void DrawConfig4(void) {
	uint32_t frames;
	
	OPEN_DISPS();
	
	frames = sGameplayFrames;
	
	gSPSegment(POLY_XLU_DISP++, 0x08,
		Gfx_TwoTexScroll(0, 127 - frames % 128, (frames * 1) % 128, 32, 32, 1, frames % 128,
		(frames * 1) % 128, 32, 32)
	);
	
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
	
	CLOSE_DISPS();
}

/**
 * SceneTableEntry Draw Config 2:
 * Has no effect, and is only used in SPOT00 (cutscene scene).
 */
static void DrawConfigDoNothing(void) {
}

/**
 * SceneTableEntry Draw Config 5:
 * This config is unused, and its purpose is unknown.
 */
static void DrawConfig5(void) {
	uint32_t dListIndex;
	uint32_t alpha;
	
	if (sPlayRoomCtxUnk7A[0] != 0) {
		dListIndex = 1;
		alpha = sPlayRoomCtxUnk7A[1];
	} else {
		dListIndex = 0;
		alpha = 255;
	}
	
	if (alpha == 0) {
		sPlayRoomCtxUnk78 = 0;
	} else {
		OPEN_DISPS();
		
		sPlayRoomCtxUnk78 = 1;
		AnimatedMat_Draw(sSceneMaterialAnims);
		Scene_SetRenderModeXlu(dListIndex, 3);
		gDPSetEnvColor(POLY_OPA_DISP++, 255, 255, 255, alpha);
		gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, alpha);
		
		CLOSE_DISPS();
	}
}

/**
 * SceneTableEntry Draw Config 6:
 * This is a special draw config for Great Bay Temple, which handles both material animations as well as setting the lod
 * fraction to a certain value when certain flags are set, which are likely used for the pipes whenever they are
 * activated.
 */
static void DrawConfigGreatBayTemple(void) {
	static Gfx greatBayTempleColorSetDL[] = {
		gsDPSetPrimColor(0, 255, 255, 255, 255, 255),
		gsSPEndDisplayList(),
	};
	int32_t lodFrac;
	int32_t i;
	Gfx* gfx;
	Gfx* gfxHead;
	
	/*
	if (Flags_GetSwitch(play, 0x33) && Flags_GetSwitch(play, 0x34) && Flags_GetSwitch(play, 0x35) &&
		Flags_GetSwitch(play, 0x36)) {
		BgCheck_SetContextFlags(&play->colCtx, BGCHECK_FLAG_REVERSE_CONVEYOR_FLOW);
	} else {
		BgCheck_UnsetContextFlags(&play->colCtx, BGCHECK_FLAG_REVERSE_CONVEYOR_FLOW);
	}
	*/
	
	gfxHead = GRAPH_ALLOC(18 * sizeof(Gfx));
	
	AnimatedMat_Draw(sSceneMaterialAnims);
	
	OPEN_DISPS();
	
	for (gfx = gfxHead, i = 0; i < 9; i++, gfx += 2)
	{
		lodFrac = 0;
		
		memcpy(gfx, greatBayTempleColorSetDL, sizeof(greatBayTempleColorSetDL));
		
		/*
		switch (i) {
			case 0:
				if (Flags_GetSwitch(play, 0x33) && Flags_GetSwitch(play, 0x34) && Flags_GetSwitch(play, 0x35) &&
					Flags_GetSwitch(play, 0x36)) {
					lodFrac = 255;
				}
				break;
			case 1:
				if (Flags_GetSwitch(play, 0x37)) {
					lodFrac = 68;
				}
				break;
			case 2:
				if (Flags_GetSwitch(play, 0x37) && Flags_GetSwitch(play, 0x38)) {
					lodFrac = 68;
				}
				break;
			case 3:
				if (Flags_GetSwitch(play, 0x37) && Flags_GetSwitch(play, 0x38) && Flags_GetSwitch(play, 0x39)) {
					lodFrac = 68;
				}
				break;
			case 4:
				if (!(Flags_GetSwitch(play, 0x33))) {
					lodFrac = 68;
				}
				break;
			case 5:
				if (Flags_GetSwitch(play, 0x34)) {
					lodFrac = 68;
				}
				break;
			case 6:
				if (Flags_GetSwitch(play, 0x34) && Flags_GetSwitch(play, 0x35)) {
					lodFrac = 68;
				}
				break;
			case 7:
				if (Flags_GetSwitch(play, 0x34) && Flags_GetSwitch(play, 0x35) && Flags_GetSwitch(play, 0x36)) {
					lodFrac = 68;
				}
				break;
			case 8:
				if (Flags_GetSwitch(play, 0x3A)) {
					lodFrac = 68;
				}
				break;
		}
		*/
		
		gDPSetPrimColor(gfx, 0, lodFrac, 255, 255, 255, 255);
	}
	
	gSPSegment(POLY_OPA_DISP++, 0x06, gfxHead);
	gSPSegment(POLY_XLU_DISP++, 0x06, gfxHead);
	
	CLOSE_DISPS();
}

/**
 * SceneTableEntry Draw Config 7:
 * This is a special draw config for Sakon's Hideout, as well as the Music Box House. Its step value is set manually
 * rather than always animating like `DrawConfigMatAnim`.
 */
static void DrawConfigMatAnimManualStep(void) {
	AnimatedMat_DrawStep(sSceneMaterialAnims, sPlayRoomCtxUnk7A[0]);
}

#endif // endregion

#if 1 // region: public animatedmat

/**
 * Draws an animated material to both OPA and XLU buffers.
 */
void AnimatedMat_Draw(AnimatedMaterial* matAnim) {
	AnimatedMat_DrawMain(matAnim, 1, sGameplayFrames, 3);
}

/**
 * Draws an animated material to only the OPA buffer.
 */
void AnimatedMat_DrawOpa(AnimatedMaterial* matAnim) {
	AnimatedMat_DrawMain(matAnim, 1, sGameplayFrames, 1);
}

/**
 * Draws an animated material to only the XLU buffer.
 */
void AnimatedMat_DrawXlu(AnimatedMaterial* matAnim) {
	AnimatedMat_DrawMain(matAnim, 1, sGameplayFrames, 2);
}

/**
 * Draws an animated material with an alpha ratio (0.0 - 1.0) both OPA and XLU buffers.
 */
void AnimatedMat_DrawAlpha(AnimatedMaterial* matAnim, float alphaRatio) {
	AnimatedMat_DrawMain(matAnim, alphaRatio, sGameplayFrames, 3);
}

/**
 * Draws an animated material with an alpha ratio (0.0 - 1.0) to only the OPA buffer.
 */
void AnimatedMat_DrawAlphaOpa(AnimatedMaterial* matAnim, float alphaRatio) {
	AnimatedMat_DrawMain(matAnim, alphaRatio, sGameplayFrames, 1);
}

/**
 * Draws an animated material with an alpha ratio (0.0 - 1.0) to only the XLU buffer.
 */
void AnimatedMat_DrawAlphaXlu(AnimatedMaterial* matAnim, float alphaRatio) {
	AnimatedMat_DrawMain(matAnim, alphaRatio, sGameplayFrames, 2);
}

/**
 * Draws an animated material with a step to both the OPA and XLU buffers.
 */
void AnimatedMat_DrawStep(AnimatedMaterial* matAnim, texAnimStep_t step) {
	AnimatedMat_DrawMain(matAnim, 1, step, 3);
}

/**
 * Draws an animated material with a step to only the OPA buffer.
 */
void AnimatedMat_DrawStepOpa(AnimatedMaterial* matAnim, texAnimStep_t step) {
	AnimatedMat_DrawMain(matAnim, 1, step, 1);
}

/**
 * Draws an animated material with a step to only the XLU buffer.
 */
void AnimatedMat_DrawStepXlu(AnimatedMaterial* matAnim, texAnimStep_t step) {
	AnimatedMat_DrawMain(matAnim, 1, step, 2);
}

/**
 * Draws an animated material with an alpha ratio (0.0 - 1.0) and a step to both the OPA and XLU buffers.
 */
void AnimatedMat_DrawAlphaStep(AnimatedMaterial* matAnim, float alphaRatio, texAnimStep_t step) {
	AnimatedMat_DrawMain(matAnim, alphaRatio, step, 3);
}

/**
 * Draws an animated material with an alpha ratio (0.0 - 1.0) and a step to only the OPA buffer.
 */
void AnimatedMat_DrawAlphaStepOpa(AnimatedMaterial* matAnim, float alphaRatio, texAnimStep_t step) {
	AnimatedMat_DrawMain(matAnim, alphaRatio, step, 1);
}

/**
 * Draws an animated material with an alpha ratio (0.0 - 1.0) and a step to only the XLU buffer.
 */
void AnimatedMat_DrawAlphaStepXlu(AnimatedMaterial* matAnim, float alphaRatio, texAnimStep_t step) {
	AnimatedMat_DrawMain(matAnim, alphaRatio, step, 2);
}
#endif // endregion

#if 1 // region: remaining public api
/**
 * Executes the current scene draw config handler.
 */
void TexAnimSetupSceneMM(int which, AnimatedMaterial *sceneMaterialAnims)
{
	static void (*sceneDrawConfigHandlers[])() = {
		DrawConfigDefault,
		DrawConfigMatAnim,
		DrawConfigDoNothing,
		DrawConfig3, // unused, leftover from OoT
		DrawConfig4, // unused, leftover from OoT
		DrawConfig5, // unused
		DrawConfigGreatBayTemple,
		DrawConfigMatAnimManualStep,
	};
	
	sSceneMaterialAnims = sceneMaterialAnims;
	
	sceneDrawConfigHandlers[which]();
}

void TexAnimSetGameplayFrames(float frames)
{
	sGameplayFrames = frames;
}

AnimatedMaterial *AnimatedMaterialNewFromSegment(uint32_t segAddr)
{
	sb_array(AnimatedMaterial, result) = 0;
	AnimatedMaterial64 *matAnim = n64_segment_get(segAddr);
	
	#define READY(type) \
		type##64 *in = data; \
		type *out = (type*)calloc(1, sizeof(*out)); \
		mat.params = out;
	
	#define READY_ARRAY(type, howmany) \
		type##64 *in = data; \
		type *out = (type*)calloc(howmany, sizeof(*out)); \
		mat.params = out;
	
	#define READY_PTR(type, name) \
		type *name = n64_segment_get(u32r(&in->name));
	
	if ((matAnim != NULL) && (matAnim->segment != 0))
	{
		for (int segment = 0; segment >= 0; ++matAnim)
		{
			AnimatedMaterial mat = { matAnim->segment, u16r(&matAnim->type) };
			void *data = n64_segment_get(u32r(&matAnim->params));
			
			//fprintf(stderr, "push type %d seg %d\n", mat.type, mat.segment);
			
			segment = mat.segment;
			
			switch (mat.type)
			{
				case 0: // AnimatedMat_DrawTexScroll
				case 1: // AnimatedMat_DrawTwoTexScroll
				{
					READY_ARRAY(AnimatedMatTexScrollParams, mat.type + 1)
					
					for (int i = 0; i < mat.type + 1; ++i, ++out, ++in)
					{
						out->xStep = in->xStep;
						out->yStep = in->yStep;
						out->width = in->width;
						out->height = in->height;
					}
					break;
				}
				
				case 2: // AnimatedMat_DrawColor
				case 3: // AnimatedMat_DrawColorLerp
				case 4: // AnimatedMat_DrawColorNonLinearInterp
				{
					READY(AnimatedMatColorParams)
					READY_PTR(F3DPrimColor, primColors)
					READY_PTR(F3DEnvColor, envColors)
					READY_PTR(uint16_t, keyFrames)
					
					out->durationFrames = u16r(&in->durationFrames);
					out->keyFrameCount = u16r(&in->keyFrameCount);
					// safety
					if (!out->keyFrameCount) {
						out->keyFrameCount = out->durationFrames;
						for (int i = 0; i < out->durationFrames; ++i)
							sb_push(out->keyFrames, i + 1);
					}
					if (primColors)
						for (int i = 0; i < out->keyFrameCount; ++i)
							sb_push(out->primColors, primColors[i]);
					if (envColors)
						for (int i = 0; i < out->keyFrameCount; ++i)
							sb_push(out->envColors, envColors[i]);
					if (keyFrames && !out->keyFrames)
						for (int i = 0; i < out->keyFrameCount; ++i)
							sb_push(out->keyFrames, u16r(&keyFrames[i]));
					break;
				}
				
				case 5: // AnimatedMat_DrawTexCycle
				{
					READY(AnimatedMatTexCycleParams)
					READY_PTR(uint32_t, textureList)
					READY_PTR(uint8_t, textureIndexList)
					int textureIndexMax = 0;
					
					out->durationFrames = u16r(&in->durationFrames);
					if (textureIndexList) {
						for (int i = 0; i < out->durationFrames; ++i) {
							int textureIndex = textureIndexList[i];
							if (textureIndex > textureIndexMax)
								textureIndexMax = textureIndex + 1;
							sb_push(out->textureIndexList, textureIndex);
						}
					}
					for (int i = 0; i < textureIndexMax; ++i)
						sb_push(out->textureList, u32r(&textureList[i]));
					break;
				}
			}
			
			sb_push(result, mat);
		}
	}
	
	return result;
}

#endif // endregion
