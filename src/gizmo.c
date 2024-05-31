
#include <stdlib.h>
#include <stdio.h>

#include "gizmo.h"
#include "incbin.h"
#include "extmath.h"
#include "n64.h"
#include "misc.h"
#include "window.h"

// assets
INCBIN(GfxGizmo, "embed/gfxGizmo.bin");

// types
struct Gizmo
{
	Vec3f pos;
	Matrix mtx;
	bool isInUse;
};

struct Gizmo *GizmoNew(void)
{
	struct Gizmo *gizmo = calloc(1, sizeof(*gizmo));
	
	return gizmo;
}

void GizmoSetPosition(struct Gizmo *gizmo, float x, float y, float z)
{
	gizmo->pos = (Vec3f){x, y, z};
}

// TODO still very WIP
void GizmoDraw(struct Gizmo *gizmo, struct CameraFly *camera)
{
	#if 0
	Vec3f mxo[3] = {
		/* MXO_R */ { gizmo->mtx.xx, gizmo->mtx.yx, gizmo->mtx.zx },
		/* MXO_U */ { gizmo->mtx.xy, gizmo->mtx.yy, gizmo->mtx.zy },
		/* MXO_F */ { gizmo->mtx.xz, gizmo->mtx.yz, gizmo->mtx.zz },
	};
	static Vec3f sOffsetMul[] = {
		{ 0, 120, 0 },
		{ 0, 400, 0 },
	};
	#endif
	static GbiGfx *gfxHead = 0;
	static GbiGfx *gfxDisp;
	
	if (!gfxHead)
		gfxHead = malloc(128 * sizeof(*gfxHead));
	
	//fprintf(stderr, "draw gizmo at %f %f %f\n", UnfoldVec3(gizmo->pos));
	
	// scale
	float s = 0.25;
	
	// want gizmo to remain the same size regardless of camera distance
	if (true)
	{
		Vec3f p = Vec3f_ProjectAlong(gizmo->pos, camera->eye, camera->lookAt);
		f32 dist = Vec3f_DistXYZ(p, camera->eye);
		
		//if (camera->isOrthographic)
		//	s = camera->dist / 2850.0f;
		//else
			s = dist / 2000.0f * tanf(DegToRad(camera->fovy) / 2.0f);
	}
	
	// construct model
	gfxDisp = gfxHead;
	gSPSegment(gfxDisp++, 0x06, gGfxGizmoData);
	for (int i = 0; i < 3; i++)
	{
		Vec3f color = Vec3f_MulVal(Vec3fRGBfromHSV(1.0f - (i / 3.0f), 0.75f, 0.62f), 255);
		
		// TODO draw axis line
		if (gizmo->isInUse)
			continue;
		
		Matrix_Push(); {
			Matrix_Translate(UnfoldVec3(gizmo->pos), MTXMODE_NEW);
			Matrix_Scale(s, s, s, MTXMODE_APPLY);
			//Matrix_Mult(&gizmo->mtx, MTXMODE_APPLY);
			
			// different arrows point different directions
			if (i == 0) {
				Matrix_RotateY_d(180, MTXMODE_APPLY);
				Matrix_RotateZ_d(90, MTXMODE_APPLY);
			}
			else if (i == 2)
				Matrix_RotateX_d(90, MTXMODE_APPLY);
			
			//Matrix_MultVec3f(&sOffsetMul[0], &gizmo->cyl[i].start);
			//Matrix_MultVec3f(&sOffsetMul[1], &gizmo->cyl[i].end);
			
			//Vec3f dir = Vec3f_LineSegDir(gizmo->cyl[i].start, gizmo->cyl[i].end);
			//Vec3f frn = Vec3f_LineSegDir(camera->eye, camera->at);
			//f32 dot = invertf(fabsf(powf(Vec3f_Dot(dir, frn), 3)));
			//u8 alpha = gizmo->focus.axis[i] ? 0xFF : 0xFF * dot;
			u8 alpha = 255;
			
			gDPSetEnvColor(gfxDisp++, UNFOLD_VEC3(color), alpha);
			
			//if (gizmo->focus.axis[i])
			//	gXPSetHighlightColor(gfxDisp++, 0xFF, 0xFF, 0xFF, 0x40, DODGE);
			
			//gizmo->cyl[i].r = Vec3f_DistXYZ(gizmo->pos, camera->eye) * 0.02f;
			
			gSPMatrix(gfxDisp++, Matrix_NewMtxN64(), G_MTX_MODELVIEW | G_MTX_LOAD);
			gSPDisplayList(gfxDisp++, 0x06000810); // gGizmo_DlGizmo
			
			//if (gizmo->focus.axis[i])
			//	gXPClearHighlightColor(gfxDisp++);
		} Matrix_Pop();
	}
	gSPEndDisplayList(gfxDisp++);
	
	// display model
	n64_draw_dlist(gfxHead);
}
