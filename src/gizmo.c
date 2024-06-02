
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "gizmo.h"
#include "incbin.h"
#include "extmath.h"
#include "n64.h"
#include "misc.h"
#include "window.h"
#include "gui.h"
#include "stretchy_buffer.h"

// assets
INCBIN(GfxGizmo, "embed/gfxGizmo.bin");

enum GizmoState
{
	GIZMO_STATE_IDLE, // waiting on user action
	GIZMO_STATE_MOVE, // is being moved by user
};

struct GizmoChild
{
	Vec3f *pos;
	Vec3f startPos;
};

// types
struct Gizmo
{
	Vec3f pos;
	Vec3f actionPos; // position of gizmo at time of last state change
	Vec2f actionCursorPos;
	Matrix mtx;
	struct GizmoAxis {
		Cylinder cylinder;
		bool isHovered;
		bool isLocked;
	} axis[3];
	struct GizmoKeyboard {
		float value;
		bool isActive;
	} keyboard;
	bool isInUse;
	enum GizmoState state;
	sb_array(struct GizmoChild, children);
};

#if 1 // region: private functions

static void GizmoSetState(struct Gizmo *gizmo, enum GizmoState newState)
{
	gizmo->state = newState;
	gizmo->actionPos = gizmo->pos;
	gizmo->actionCursorPos = (Vec2f) { UnfoldVec2(gInput.mouse.pos) };
}

// rayPos is the result of a raycast against the geometry in the game world
static void GizmoMove(struct Gizmo *gizmo, Vec3f *rayPos)
{
	bool ctrlHold = gInput.key.lctrl;
	Vec3f mxo[3] = {
		/* Right */ { gizmo->mtx.xx, gizmo->mtx.yx, gizmo->mtx.zx },
		/* Up    */ { gizmo->mtx.xy, gizmo->mtx.yy, gizmo->mtx.zy },
		/* Front */ { gizmo->mtx.xz, gizmo->mtx.yz, gizmo->mtx.zz },
	};
	struct GizmoKeyboard *keyboard = &gizmo->keyboard;
	struct GizmoAxis *axis = gizmo->axis;
	bool isAnyAxisLocked = (axis[0].isLocked || axis[1].isLocked || axis[2].isLocked);
	
	// entering movement axis and distance using keyboard
	if (keyboard->isActive && keyboard->value && isAnyAxisLocked)
	{
		gizmo->pos.x = gizmo->actionPos.x + keyboard->value * axis[0].isLocked;
		gizmo->pos.y = gizmo->actionPos.y + keyboard->value * axis[1].isLocked;
		gizmo->pos.z = gizmo->actionPos.z + keyboard->value * axis[2].isLocked;
	}
	else
	{
		Vec2f gizmoScreenSpace = WindowGetLocalScreenPos(gizmo->pos);
		Vec2f mv = Vec2f_New(UnfoldVec2(gInput.mouse.vel));
		RayLine curRay = WindowGetRayLine(gizmoScreenSpace);
		RayLine nextRay = WindowGetRayLine(Vec2f_Add(gizmoScreenSpace, mv));
		
		// axis constrained movement
		if (isAnyAxisLocked)
		{
			for (int i = 0; i < 3; ++i)
			{
				if (gizmo->axis[i].isLocked)
				{
					gizmo->pos = Vec3f_ClosestPointOnRay(
						nextRay.start
						, nextRay.end
						, gizmo->pos
						, Vec3f_Add(gizmo->pos, mxo[i])
					);
					break;
				}
			}
			
			// old method of axis lock; seems to function no
			// differently from the code i added to the bottom
			// of this function
			/*
			if (ctrlHold && rayPos)
			{
				for (int i = 0; i < 3; ++i)
				{
					if (!gizmo->axis[i].isLocked)
						continue;
					
					Vec3f off = Vec3f_Project(Vec3f_Sub(*rayPos, gizmo->pos), mxo[i]);
					
					gizmo->pos = Vec3f_Add(gizmo->pos, off);
				}
			}
			*/
		}
		// unconstrained movement
		else
		{
			Vec3f nextRayN = Vec3f_LineSegDir(nextRay.start, nextRay.end);
			float nextDist = Vec3f_DistXYZ(nextRay.start, nextRay.end);
			float curDist = Vec3f_DistXYZ(curRay.start, curRay.end);
			float pointDist = Vec3f_DistXYZ(curRay.start, gizmo->pos);
			float distRatio = nextDist / curDist;
			
			gizmo->pos = Vec3f_Add(nextRay.start, Vec3f_MulVal(nextRayN, pointDist * distRatio));
		}
	}
	
	// holding ctrl should snap to the world geometry
	if (ctrlHold && rayPos)
	{
		// respect axis locks when moving
		if (!isAnyAxisLocked || axis[0].isLocked)
			gizmo->pos.x = rintf(rayPos->x);
		if (!isAnyAxisLocked || axis[1].isLocked)
			gizmo->pos.y = rintf(rayPos->y);
		if (!isAnyAxisLocked || axis[2].isLocked)
			gizmo->pos.z = rintf(rayPos->z);
	}
	
	/*
	fornode(elem, gizmo->elemHead) {
		Vec3f relPos = Vec3f_Sub(gizmo->pos, gizmo->actionPos);
		
		elem->interact = true;
		elem->pos = Vec3f_Add(*elem->dpos, relPos);
		
		for (var_t i  = 0; i < 3; i++)
			elem->pos.axis[i] = rint(elem->pos.axis[i]);
	}
	*/
}

// update children with gizmo movement
static void GizmoUpdateChildren(struct Gizmo *gizmo)
{
	sb_foreach(gizmo->children, {
		*(each->pos) = Vec3f_Add(each->startPos, Vec3f_Sub(gizmo->pos, gizmo->actionPos));
	});
}

// apply current positions to all children
static void GizmoApplyChildren(struct Gizmo *gizmo)
{
	sb_foreach(gizmo->children, {
		each->startPos = *(each->pos);
	});
}

#endif // endregion

struct Gizmo *GizmoNew(void)
{
	struct Gizmo *gizmo = calloc(1, sizeof(*gizmo));
	
	return gizmo;
}

void GizmoFree(struct Gizmo *gizmo)
{
	sb_free(gizmo->children);
	
	free(gizmo);
}

bool GizmoHasFocus(struct Gizmo *gizmo)
{
	return gizmo->state != GIZMO_STATE_IDLE;
}

bool GizmoIsHovered(struct Gizmo *gizmo)
{
	for (int i = 0; i < 3; ++i)
		if (gizmo->axis[i].isHovered)
			return true;
	
	return false;
}

void GizmoAddChild(struct Gizmo *gizmo, Vec3f *childPos)
{
	struct GizmoChild child = {
		.pos = childPos
		, .startPos = *childPos
	};
	
	sb_push(gizmo->children, child);
}

void GizmoRemoveChildren(struct Gizmo *gizmo)
{
	sb_clear(gizmo->children);
}

void GizmoSetPosition(struct Gizmo *gizmo, float x, float y, float z)
{
	gizmo->pos = (Vec3f){x, y, z};
}

void GizmoUpdate(struct Gizmo *gizmo, Vec3f *rayPos)
{
	// global gizmo orientation
	Matrix_Clear(&gizmo->mtx);
	
	if (gInput.mouse.isControllingCamera)
		return;
	
	if (!sb_count(gizmo->children))
		return;
	
	if (gizmo->state == GIZMO_STATE_IDLE)
	{
		RayLine ray = WindowGetCursorRayLine();
		
		// idle = no movement
		gizmo->actionPos = gizmo->pos;
		
		// press G key to move, like Blender
		if (gInput.key.g)
			GizmoSetState(gizmo, GIZMO_STATE_MOVE);
		
		for (int i = 0; i < 3; i++)
		{
			struct GizmoAxis *axis = &gizmo->axis[i];
			bool isHovered = Col3D_LineVsCylinder(&ray, &axis->cylinder, 0);
			
			axis->isHovered = isHovered;
			
			if (isHovered)
			{
				if (gInput.mouse.button.left)
				{
					GizmoSetState(gizmo, GIZMO_STATE_MOVE);
					
					// only lock one axis on click
					for (int k = 0; k < 3; ++k)
						gizmo->axis[k].isLocked = false;
					gizmo->axis[i].isLocked = true;
					
					break;
				}
			}
		}
	}
	else if (gizmo->state == GIZMO_STATE_MOVE)
		GizmoMove(gizmo, rayPos);
	
	GizmoUpdateChildren(gizmo);
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
	
	if (!sb_count(gizmo->children))
		return;
	
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
		
		// hide axis lines if the gizmo is being manipulated by the user
		if (gizmo->state)
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
	
	if (gizmo->state == GIZMO_STATE_MOVE)
	{
		if (gInput.mouse.button.left)
		{
			for (int i = 0; i < 3; i++)
			{
				if (!gizmo->axis[i].isLocked)
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
		else if (gInput.mouseOld.button.left)
		{
			// mouse button has been released
			gizmo->state = GIZMO_STATE_IDLE;
			
			for (int i = 0; i < 3; ++i)
				gizmo->axis[i].isLocked = false;
			
			GizmoApplyChildren(gizmo);
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
