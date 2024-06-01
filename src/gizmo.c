
#include <stdlib.h>
#include <stdio.h>

#include "gizmo.h"
#include "incbin.h"
#include "extmath.h"
#include "n64.h"
#include "misc.h"
#include "window.h"
#include "gui.h"

// assets
INCBIN(GfxGizmo, "embed/gfxGizmo.bin");

// types
struct Gizmo
{
	Vec3f pos;
	Matrix mtx;
	struct GizmoAxis {
		Cylinder cylinder;
		bool isHovered;
	} axis[3];
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

void GizmoUpdate(struct Gizmo *gizmo)
{
	// global gizmo orientation
	Matrix_Clear(&gizmo->mtx);
	
	if (true/*!gizmo->lock.state*/)
	{
		RayLine ray = WindowGetCursorRayLine();
		
		for (int i = 0; i < 3; i++)
		{
			struct GizmoAxis *axis = &gizmo->axis[i];
			bool isHovered = Col3D_LineVsCylinder(&ray, &axis->cylinder, 0);
			
			axis->isHovered = isHovered;
			
			/*
			if (isHovered)
			{
				if (Input_GetCursor(input, CLICK_L)->press)
				{
					Gizmo_SetAction(gizmo, GIZMO_ACTION_MOVE);
					gizmo->initpos = gizmo->pos;
					gizmo->lock.axis[i] = true;
					gizmo->initAction = true;
					gizmo->pressHold = true;
					break;
				}
			}
			*/
		}
	}
}

// TODO still very WIP
void GizmoDraw(struct Gizmo *gizmo, struct CameraFly *camera)
{
	Vec3f mxo[3] = {
		/* MXO_R */ { gizmo->mtx.xx, gizmo->mtx.yx, gizmo->mtx.zx },
		/* MXO_U */ { gizmo->mtx.xy, gizmo->mtx.yy, gizmo->mtx.zy },
		/* MXO_F */ { gizmo->mtx.xz, gizmo->mtx.yz, gizmo->mtx.zz },
	};
	static Vec3f sOffsetMul[] = {
		{ 0, 120, 0 },
		{ 0, 400, 0 },
	};
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
		struct GizmoAxis *axis = &gizmo->axis[i];
		struct Cylinder *cyl = &(axis->cylinder);
		
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
			
			Matrix_MultVec3f(&sOffsetMul[0], &cyl->start);
			Matrix_MultVec3f(&sOffsetMul[1], &cyl->end);
			
			// fancy translucency
			Vec3f dir = Vec3f_LineSegDir(cyl->start, cyl->end);
			Vec3f frn = Vec3f_LineSegDir(camera->eye, camera->lookAt);
			f32 dot = invertf(fabsf(powf(Vec3f_Dot(dir, frn), 3)));
			u8 alpha = axis->isHovered ? 0xFF : 0xFF * dot;
			//alpha = 255; // opaque
			
			gDPSetEnvColor(gfxDisp++, UNFOLD_VEC3(color), alpha);
			
			if (axis->isHovered)
				gXPSetHighlightColor(gfxDisp++, 0xFF, 0xFF, 0xFF, 0x40, DODGE);
			
			cyl->r = Vec3f_DistXYZ(gizmo->pos, camera->eye) * 0.02f;
			
			gSPMatrix(gfxDisp++, Matrix_NewMtxN64(), G_MTX_MODELVIEW | G_MTX_LOAD);
			gSPDisplayList(gfxDisp++, 0x06000810); // gGizmo_DlGizmo
			
			if (axis->isHovered)
				gXPClearHighlightColor(gfxDisp++);
		} Matrix_Pop();
	}
	gSPEndDisplayList(gfxDisp++);
	
	// display model
	n64_draw_dlist(gfxHead);
	
	if (true/*gizmo->action && gizmo->activeView == gizmo->curView*/)
	{
		if (true/*gizmo->lock.state && gizmo->lock.state != GIZMO_AXIS_ALL_TRUE*/)
		{
			for (int i = 0; i < 3; i++)
			{
				if (!gizmo->axis[i].isHovered)
					continue;
				
				Vec3f aI = Vec3f_Add(gizmo->pos, Vec3f_MulVal(mxo[i], 1000));
				Vec3f bI = Vec3f_Add(gizmo->pos, Vec3f_MulVal(mxo[i], -1000));
				Vec2f aO, bO;
				
				WindowClipPointIntoView(&aI, Vec3f_Invert(mxo[i]));
				WindowClipPointIntoView(&bI, mxo[i]);
				aO = WindowGetLocalScreenPos(aI);
				bO = WindowGetLocalScreenPos(bI);
				
				// draw long axis lines
				Vec3f colorVec = Vec3fRGBfromHSV(1.0f - (i / 3.0f), 0.5f, 0.5f);
				uint32_t color = (Vec3fRGBto24bit(colorVec) << 8) | 255;
				GuiPushLine(UNFOLD_VEC2(aO), UNFOLD_VEC2(bO), color, 2.0f);
			}
		}
		
		/*
		char* txt = x_fmt("%s %.8g along %s",
				gizmo->action == 1 ? "Move" : "Rotate",
				gizmo->value,
				gizmo->lock.x ? "X" : gizmo->lock.y ? "Y" : "Z"
		);
		
		nvgFontSize(vg, SPLIT_TEXT);
		nvgFontFace(vg, "default");
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
		
		nvgFontBlur(vg, 1.0f);
		nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
		nvgText(vg, 8, split->rect.h - SPLIT_TEXT_H - SPLIT_ELEM_X_PADDING, txt, NULL);
		
		nvgFontBlur(vg, 0.0f);
		nvgFillColor(vg, Theme_GetColor(THEME_TEXT, 255, 1.0f));
		nvgText(vg, 8, split->rect.h - SPLIT_TEXT_H - SPLIT_ELEM_X_PADDING, txt, NULL);
		*/
	}
}
