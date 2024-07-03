#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>

// TODO the deferred paste/snap/rotate stuff is a bit messy,
//      so do it with a queuedInstanceSnap or something later
//      (current impl is acceptable for minimum viable product)

#include "logging.h"
#include "misc.h"
#include "extmath.h"
#include "gizmo.h"
#include "gui.h"
#include "window.h"
#include "texanim.h"
#include "object.h"
#include "skelanime.h"
#include "rendercode.h"
#include <n64.h>
#include <n64types.h>

#define NOC_FILE_DIALOG_IMPLEMENTATION
#include "noc_file_dialog.h"

static GbiGfx gfxEnableXray[] = { gsXPMode(0, GX_MODE_OUTLINE), gsSPEndDisplayList() };
static GbiGfx gfxDisableXray[] = { gsXPMode(GX_MODE_OUTLINE, 0), gsSPEndDisplayList() };
static double sGameplayFrames = 0;

// a slanted triangular prism that points in one direction
unsigned char meshPrismArrow[] = {
	0x03, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x0c,
	0x24, 0x62, 0x48, 0xff, 0xff, 0x3d, 0x01, 0xb8, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x1c, 0x00, 0x0c, 0x24, 0x62, 0x48, 0xff, 0xfc, 0x18, 0x00, 0x00,
	0x03, 0xe8, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x08, 0x24, 0x62, 0x48, 0xff,
	0xfe, 0x3a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x1c,
	0x9d, 0x3a, 0x36, 0xff, 0xff, 0x3d, 0x01, 0xb8, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x14, 0x00, 0x1c, 0x9d, 0x3a, 0x36, 0xff, 0xfc, 0x18, 0x00, 0x00,
	0xfc, 0x18, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x18, 0x9d, 0x3a, 0x36, 0xff,
	0x03, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x08,
	0x00, 0x81, 0x00, 0xff, 0xfe, 0x3a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x04, 0x00, 0x0c, 0x00, 0x81, 0x00, 0xff, 0xfc, 0x18, 0x00, 0x00,
	0xfc, 0x18, 0x00, 0x00, 0x00, 0x04, 0x00, 0x10, 0x00, 0x81, 0x00, 0xff,
	0xfc, 0x18, 0x00, 0x00, 0x03, 0xe8, 0x00, 0x00, 0x00, 0x14, 0x00, 0x20,
	0x9d, 0x3a, 0xca, 0xff, 0xff, 0x3d, 0x01, 0xb8, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x14, 0x00, 0x1c, 0x9d, 0x3a, 0xca, 0xff, 0xfe, 0x3a, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x1c, 0x9d, 0x3a, 0xca, 0xff,
	0x03, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x0c,
	0x24, 0x62, 0xb8, 0xff, 0xfc, 0x18, 0x00, 0x00, 0xfc, 0x18, 0x00, 0x00,
	0x00, 0x17, 0x00, 0x10, 0x24, 0x62, 0xb8, 0xff, 0xff, 0x3d, 0x01, 0xb8,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x0c, 0x24, 0x62, 0xb8, 0xff,
	0xfc, 0x18, 0x00, 0x00, 0x03, 0xe8, 0x00, 0x00, 0x00, 0x04, 0x00, 0x08,
	0x00, 0x81, 0x00, 0xff, 0x01, 0x01, 0x00, 0x20, 0x06, 0x00, 0x00, 0x00,
	0x06, 0x00, 0x02, 0x04, 0x00, 0x06, 0x08, 0x0a, 0x06, 0x0c, 0x0e, 0x10,
	0x00, 0x12, 0x14, 0x16, 0x06, 0x18, 0x1a, 0x1c, 0x00, 0x0c, 0x1e, 0x0e,
	0xdf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// a blank material
unsigned char matBlank[] = {
	0xd9, 0xfc, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xe7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd9, 0xff, 0xff, 0xff,
	0x00, 0x03, 0x00, 0x00, 0xfc, 0xbd, 0x7e, 0x03, 0x8f, 0xfe, 0x7d, 0xf8,
	0xd9, 0xf3, 0xff, 0xff, 0x00, 0x03, 0x04, 0x00, 0xfa, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0xfb, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
	0xdf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// write bigendian bytes
#define WBE16(DST, V) { ((uint8_t*)DST)[0] = (V) >> 8; ((uint8_t*)DST)[1] = (V) & 0xff; }

#define FLYTHROUGH_CAMERA_IMPLEMENTATION
#include "flythrough_camera.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

// settings
#define WINDOW_INITIAL_WIDTH   800
#define WINDOW_INITIAL_HEIGHT  600
#define WINDOW_INITIAL_FOVY    60.0f
#define PROJ_NEAR              10

SkelAnime gSkelAnimeTest = {0};

struct Input gInput = {
	.textInput = {
		.maxChars = sizeof(gInput.textInput.text)
	}
};

static struct State
{
	struct Input *input;
	Matrix viewMtx;
	Matrix projMtx;
	Matrix projViewMtx;
	int winWidth;
	int winHeight;
	struct CameraFly cameraFly;
	bool cameraIgnoreLMB;
	bool hasCameraMoved;
	bool deferredInstancePaste;
	struct Gizmo *gizmo;
} gState = {
	.winWidth = WINDOW_INITIAL_WIDTH
	, .winHeight = WINDOW_INITIAL_HEIGHT
	, .cameraFly = {
		.fovy = WINDOW_INITIAL_FOVY
	}
};

static struct GuiInterop *gGui = 0;

struct CameraRay
{
	RayLine ray;
	Vec3f pos;
	Vec3f dir;
	uint32_t renderGroup;
	bool isSelectingInstance; // instance selection by mouse click
	struct Instance *currentInstance;
	Triangle snapAngleTri;
	bool useSnapAngle;
	uint32_t renderGroupClicked;
};
static struct CameraRay worldRayData = { 0 };

void CameraRayCallback(void *udata, const N64Tri *tri64)
{
	struct CameraRay *ud = udata;

	if ((tri64->setId >> 24) != (ud->renderGroup >> 24))
		return;
	
	Triangle tri = {
		.v = {
			{ UNFOLD_VEC3(tri64->vtx[0]->pos) }
			,{ UNFOLD_VEC3(tri64->vtx[1]->pos) }
			,{ UNFOLD_VEC3(tri64->vtx[2]->pos) }
		}
		, .cullBackface = tri64->cullBackface
		, .cullFrontface = tri64->cullFrontface
	};
	
	if (Col3D_LineVsTriangle(
		&ud->ray
		, &tri
		, &ud->pos
		, &ud->dir
		, tri64->cullBackface
		, tri64->cullFrontface
	)) {
		ud->renderGroupClicked = tri64->setId;
		ud->renderGroup &= RENDERGROUP_MASK_GROUP;
		bool isRoomGeometry = ud->renderGroup == RENDERGROUP_ROOM;
		if (worldRayData.isSelectingInstance && ud->renderGroup == RENDERGROUP_INST)
		{
			// TODO use rendergroup id to select from appropriate list (spawn/actor/door)
			//      instead of relying on global variable (global variable is fine for now,
			//      but will break if multiple instances drawn in a single n64_buffer_flush)
			struct Instance *each = ud->currentInstance;
			
			// TODO construct a list of collisions and choose the nearest instance
			// TODO if multiple instances overlap, each click should cycle through them
			GizmoSetPosition(gState.gizmo, UNFOLD_VEC3(each->pos));
			GizmoAddChild(gState.gizmo, &each->pos);
			gGui->selectedInstance = each;

			LogDebug("RENDERGROUP_INST");
		}
		ud->renderGroup |= tri64->setId & RENDERGROUP_MASK_ID;
		
		// adjust rotation when snapping to different surfaces
		if (isRoomGeometry
			&& ((gGui->selectedInstance && gInput.key.lctrl) // moving inst w/ ctrl held
				|| gState.deferredInstancePaste // auto-rotate on paste via ctrl+v
				|| gInput.mouse.clicked.right // auto-rotate on right-click->(new/paste)
			)
		)
		{
			ud->useSnapAngle = true;
			ud->snapAngleTri = tri;
		}
	}
}

void DrawDefaultActorPreview(struct Instance *inst)
{
	float scale = 0.025;
	n64_buffer_clear();
	Matrix_Push(); { // from ReadyMatrix(), might consolidate later
		Matrix_Translate(
			inst->pos.x + 0
			, inst->pos.y + 0
			, inst->pos.z + 0
			, MTXMODE_NEW
		);
		Matrix_RotateY_s(inst->yrot - 90 * 182.044444, MTXMODE_APPLY);
		Matrix_RotateX_s(inst->xrot, MTXMODE_APPLY);
		Matrix_RotateZ_s(inst->zrot, MTXMODE_APPLY);
		Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
		
		gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtxN64(), G_MTX_MODELVIEW | G_MTX_LOAD);
	} Matrix_Pop();
	gSPSegment(POLY_OPA_DISP++, 0x06, meshPrismArrow);
	gSPDisplayList(POLY_OPA_DISP++, 0x06000100);
	n64_buffer_flush(false);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
	//if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	//	glfwSetWindowShouldClose(window, true);
	
	/* time since last execution */
	{
		static double last = 0;
		double cur = glfwGetTime();
		gInput.delta_time_sec = (cur - last);
		last = cur;
	}
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
	gState.winWidth = width;
	gState.winHeight = height;
}

static void char_callback(GLFWwindow *window, unsigned int codepoint)
{
	if (gInput.textInput.isEnabled == false)
		return;
	
	StrcatCharLimit(
		gInput.textInput.text
		, codepoint
		, Min(gInput.textInput.maxChars, sizeof(gInput.textInput.text))
	);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	int pressed = action != GLFW_RELEASE;
	
	#define SHORTCUT_CHECKS (!GuiHasFocus() \
		&& action == GLFW_PRESS \
	)
	
	switch (key)
	{
		case GLFW_KEY_W:
			gInput.key.w = pressed;
			break;
		
		case GLFW_KEY_A:
			gInput.key.a = pressed;
			break;
		
		case GLFW_KEY_S:
			// ctrl + s ; Ctrl+S
			if ((mods & GLFW_MOD_CONTROL)
				&& SHORTCUT_CHECKS
			)
			{
				// save as
				if (mods & GLFW_MOD_SHIFT)
					WindowSaveSceneAs();
				// save
				else
					WindowSaveScene();
			}
			else
				gInput.key.s = pressed;
			break;
		
		case GLFW_KEY_D:
			// Shift + D to Duplicate & Move
			if (!((mods & GLFW_MOD_SHIFT)
				&& SHORTCUT_CHECKS
				&& WindowTryInstanceDuplicate()
			))
				gInput.key.d = pressed;
			break;
		
		case GLFW_KEY_C:
			// Ctrl+C to Copy
			if ((mods & GLFW_MOD_CONTROL)
				&& SHORTCUT_CHECKS
			)
				WindowTryInstanceCopy(true);
			break;
		
		case GLFW_KEY_V:
			// Ctrl+V to Paste
			if ((mods & GLFW_MOD_CONTROL)
				&& SHORTCUT_CHECKS
			)
				WindowTryInstancePaste(true, true);
			break;
		
		case GLFW_KEY_X:
			// Ctrl+X to Cut
			if ((mods & GLFW_MOD_CONTROL)
				&& SHORTCUT_CHECKS
			)
				WindowTryInstanceCut(true);
			break;
		
		case GLFW_KEY_Q:
			gInput.key.q = pressed;
			break;
		
		case GLFW_KEY_E:
			gInput.key.e = pressed;
			break;
		
		case GLFW_KEY_G:
			gInput.key.g = pressed;
			break;
		
		case GLFW_KEY_LEFT_CONTROL:
			gInput.key.lctrl = pressed;
			break;
		
		case GLFW_KEY_LEFT_SHIFT:
			gInput.key.lshift = pressed;
			break;
		
		case GLFW_KEY_ENTER:
			gInput.key.enter = pressed;
			break;
		
		case GLFW_KEY_ESCAPE:
			gInput.key.escape = pressed;
			break;
		
		// backspace key in text edit
		case GLFW_KEY_BACKSPACE:
			if (pressed && gInput.textInput.isEnabled)
				gInput.textInput.text[strlen(gInput.textInput.text) - 1] = '\0';
			break;
		
		case GLFW_KEY_DELETE:
			if (SHORTCUT_CHECKS)
				WindowTryInstanceDelete(true);
		
		case GLFW_KEY_O:
			// ctrl + o ; Ctrl+O
			if ((mods & GLFW_MOD_CONTROL)
				&& SHORTCUT_CHECKS
			)
				WindowOpenFile();
			break;
	}
}

static void drop_callback(GLFWwindow* window, int count, const char *files[])
{
	if (count != 1)
		return;
	
	const char *filename = files[0];
	const char *extension = strrchr(filename, '.');
	
	if (!extension)
		return;
	
	//LogDebug("filename = %s, extension = %s", filename, extension);
	
	char *extensionLower = Strdup(extension + 1);
	for (char *c = extensionLower; *c; ++c)
		*c = tolower(*c);
	
	if (!strcmp(extensionLower, "zscene"))
		WindowLoadScene(filename);
	else if (!strcmp(extensionLower, "zobj"))
		WindowLoadObject(filename);
	
	free(extensionLower);
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	gInput.mouse.vel.x = xpos - gInput.mouse.pos.x;
	gInput.mouse.vel.y = ypos - gInput.mouse.pos.y;
	gInput.mouse.pos.x = xpos;
	gInput.mouse.pos.y = ypos;
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	int pressed = action != GLFW_RELEASE;
	
	switch (button)
	{
		case GLFW_MOUSE_BUTTON_RIGHT:
			gInput.mouse.button.right = pressed;
			if (action == GLFW_RELEASE && gInput.mouseOld.button.right) // TODO make it timed
				gGui->selectedInstance = 0,
				gInput.mouse.clicked.right = true;
			break;
		
		case GLFW_MOUSE_BUTTON_LEFT:
			gInput.mouse.button.left = pressed;
			if (action == GLFW_RELEASE && gInput.mouseOld.button.left) // TODO make it timed
				gInput.mouse.clicked.left = true;
			break;
	}
}

static inline void *camera_flythrough(Matrix *m)
{
	/* camera magic */
	static float pos[3] = { 0.0f, 0.0f, 0.0f };
	static float look[3] = {0, 0, -1};
	const float up[3] = {0, 1, 0};
	
	static struct {
		int x;
		int y;
	} oldcursor = {0}, cursor = {0};
	
	cursor.x = gInput.mouse.pos.x;
	cursor.y = gInput.mouse.pos.y;
	
	if (gInput.mouse.button.right)
		gInput.mouse.wheel.dy = oldcursor.y - cursor.y;
	else
		gInput.mouse.wheel.dy = 0;
	
	int activated = gState.cameraIgnoreLMB ? 0 : gInput.mouse.button.left;
	float speed = 1000;
	
	flythrough_camera_update(
		pos, look, up, (void*)m
		, gInput.delta_time_sec
		, speed * (gInput.key.lshift ? 3.0f : 1.0f)
		, 0.5f * activated
		, 80.0f
		, cursor.x - oldcursor.x, cursor.y - oldcursor.y
		, gInput.key.w
		, gInput.key.a
		, gInput.key.s
		, gInput.key.d
		, gInput.key.q // up
		, gInput.key.e // down
		, 0
	);
	
	//LogDebug("%f %f %f", UNFOLD_ARRAY_3(float, look));
	memcpy(&gState.cameraFly.eye, pos, sizeof(pos));
	//memcpy(&gState.cameraFly.lookAt, look, sizeof(look));
	gState.cameraFly.lookAt = (Vec3f) {
		pos[0] + look[0] * PROJ_NEAR,
		pos[1] + look[1] * PROJ_NEAR,
		pos[2] + look[2] * PROJ_NEAR
	};
	
	const float distSpawnActorFromCamera = 250;
	gGui->newSpawnPos = (Vec3f) {
		pos[0] + look[0] * distSpawnActorFromCamera,
		pos[1] + look[1] * distSpawnActorFromCamera,
		pos[2] + look[2] * distSpawnActorFromCamera
	};
	//LogDebug("newSpawnPos = %f %f %f", UNFOLD_VEC3(gGui->newSpawnPos));
	
	oldcursor = cursor;
	
	return m;
}

static float *identity(float dst[16])
{
	float *m = dst;
	
	memset(dst, 0, sizeof(float[16]));
	
	m[0 * 4 + 0] = 1;
	m[1 * 4 + 1] = 1;
	m[2 * 4 + 2] = 1;
	m[3 * 4 + 3] = 1;
	
	return m;
}

#undef near // win32 includes break near and far
#undef far
static float *projection(Matrix *dst, float winw, float winh, float near, float far, float fovy)
{
	float *m = (void*)dst;
	float aspect;
	float f;
	float iNF;

	/* intialize projection matrix */
	aspect = winw / winh;
	f      = 1.0 / tan(fovy * (3.14159265359f / 180.0f) * 0.5f);
	iNF    = 1.0 / (near - far);

	m[0]=f/aspect; m[4]=0.0f; m[ 8]= 0.0f;           m[12]=0.0f;
	m[1]=0.0f;     m[5]=f;    m[ 9]= 0.0f;           m[13]=0.0f;
	m[2]=0.0f;     m[6]=0.0f; m[10]= (far+near)*iNF; m[14]=2.0f*far*near*iNF;
	m[3]=0.0f;     m[7]=0.0f; m[11]=-1.0f;           m[15]=0.0f;
	
	return m;
}

typedef struct N64_ATTR_BIG_ENDIAN ZeldaMatrix {
	int32_t m[4][4];
	struct N64_ATTR_BIG_ENDIAN {
		uint16_t intPart[4][4];
		uint16_t fracPart[4][4];
	};
} ZeldaMatrix;

void mtx_to_zmtx(Matrix* src, ZeldaMatrix* dest) {
	int temp;
	uint16_t* m1 = (void*)(((char*)dest));
	uint16_t* m2 = (void*)(((char*)dest) + 0x20);
	
	temp = src->xx * 0x10000;
	m1[0] = (temp >> 0x10);
	m1[16 + 0] = temp & 0xFFFF;
	
	temp = src->yx * 0x10000;
	m1[1] = (temp >> 0x10);
	m1[16 + 1] = temp & 0xFFFF;
	
	temp = src->zx * 0x10000;
	m1[2] = (temp >> 0x10);
	m1[16 + 2] = temp & 0xFFFF;
	
	temp = src->wx * 0x10000;
	m1[3] = (temp >> 0x10);
	m1[16 + 3] = temp & 0xFFFF;
	
	temp = src->xy * 0x10000;
	m1[4] = (temp >> 0x10);
	m1[16 + 4] = temp & 0xFFFF;
	
	temp = src->yy * 0x10000;
	m1[5] = (temp >> 0x10);
	m1[16 + 5] = temp & 0xFFFF;
	
	temp = src->zy * 0x10000;
	m1[6] = (temp >> 0x10);
	m1[16 + 6] = temp & 0xFFFF;
	
	temp = src->wy * 0x10000;
	m1[7] = (temp >> 0x10);
	m1[16 + 7] = temp & 0xFFFF;
	
	temp = src->xz * 0x10000;
	m1[8] = (temp >> 0x10);
	m1[16 + 8] = temp & 0xFFFF;
	
	temp = src->yz * 0x10000;
	m1[9] = (temp >> 0x10);
	m2[9] = temp & 0xFFFF;
	
	temp = src->zz * 0x10000;
	m1[10] = (temp >> 0x10);
	m2[10] = temp & 0xFFFF;
	
	temp = src->wz * 0x10000;
	m1[11] = (temp >> 0x10);
	m2[11] = temp & 0xFFFF;
	
	temp = src->xw * 0x10000;
	m1[12] = (temp >> 0x10);
	m2[12] = temp & 0xFFFF;
	
	temp = src->yw * 0x10000;
	m1[13] = (temp >> 0x10);
	m2[13] = temp & 0xFFFF;
	
	temp = src->zw * 0x10000;
	m1[14] = (temp >> 0x10);
	m2[14] = temp & 0xFFFF;
	
	temp = src->ww * 0x10000;
	m1[15] = (temp >> 0x10);
	m2[15] = temp & 0xFFFF;
}

#if 1 /* region: day/night */

#define LERP(x, y, scale) (((y) - (x)) * (scale) + (x))
#define LERP32(x, y, scale) ((int32_t)(((y) - (x)) * (scale)) + (x))
#define LERP16(x, y, scale) ((int16_t)(((y) - (x)) * (scale)) + (x))
#define ARRAY_COUNT(X) (sizeof(X) / sizeof((X)[0]))
#define CLOCK_TIME(hr, min) ((int32_t)(((hr) * 60 + (min)) * (float)0x10000 / (24 * 60) + 0.5f))

typedef struct {
	/* 0x00 */ uint16_t startTime;
	/* 0x02 */ uint16_t endTime;
	/* 0x04 */ uint8_t lightSetting;
	/* 0x05 */ uint8_t nextLightSetting;
} TimeBasedLightEntry; // size = 0x6

TimeBasedLightEntry sTimeBasedLightConfigs[][7] = {
	{
		{ CLOCK_TIME(0, 0), CLOCK_TIME(4, 0) + 1, 3, 3 },
		{ CLOCK_TIME(4, 0) + 1, CLOCK_TIME(6, 0), 3, 0 },
		{ CLOCK_TIME(6, 0), CLOCK_TIME(8, 0) + 1, 0, 1 },
		{ CLOCK_TIME(8, 0) + 1, CLOCK_TIME(16, 0), 1, 1 },
		{ CLOCK_TIME(16, 0), CLOCK_TIME(17, 0) + 1, 1, 2 },
		{ CLOCK_TIME(17, 0) + 1, CLOCK_TIME(19, 0) + 1, 2, 3 },
		{ CLOCK_TIME(19, 0) + 1, CLOCK_TIME(24, 0) - 1, 3, 3 },
	},
	{
		{ CLOCK_TIME(0, 0), CLOCK_TIME(4, 0) + 1, 7, 7 },
		{ CLOCK_TIME(4, 0) + 1, CLOCK_TIME(6, 0), 7, 4 },
		{ CLOCK_TIME(6, 0), CLOCK_TIME(8, 0) + 1, 4, 5 },
		{ CLOCK_TIME(8, 0) + 1, CLOCK_TIME(16, 0), 5, 5 },
		{ CLOCK_TIME(16, 0), CLOCK_TIME(17, 0) + 1, 5, 6 },
		{ CLOCK_TIME(17, 0) + 1, CLOCK_TIME(19, 0) + 1, 6, 7 },
		{ CLOCK_TIME(19, 0) + 1, CLOCK_TIME(24, 0) - 1, 7, 7 },
	},
	{
		{ CLOCK_TIME(0, 0), CLOCK_TIME(4, 0) + 1, 11, 11 },
		{ CLOCK_TIME(4, 0) + 1, CLOCK_TIME(6, 0), 11, 8 },
		{ CLOCK_TIME(6, 0), CLOCK_TIME(8, 0) + 1, 8, 9 },
		{ CLOCK_TIME(8, 0) + 1, CLOCK_TIME(16, 0), 9, 9 },
		{ CLOCK_TIME(16, 0), CLOCK_TIME(17, 0) + 1, 9, 10 },
		{ CLOCK_TIME(17, 0) + 1, CLOCK_TIME(19, 0) + 1, 10, 11 },
		{ CLOCK_TIME(19, 0) + 1, CLOCK_TIME(24, 0) - 1, 11, 11 },
	},
	{
		{ CLOCK_TIME(0, 0), CLOCK_TIME(4, 0) + 1, 15, 15 },
		{ CLOCK_TIME(4, 0) + 1, CLOCK_TIME(6, 0), 15, 12 },
		{ CLOCK_TIME(6, 0), CLOCK_TIME(8, 0) + 1, 12, 13 },
		{ CLOCK_TIME(8, 0) + 1, CLOCK_TIME(16, 0), 13, 13 },
		{ CLOCK_TIME(16, 0), CLOCK_TIME(17, 0) + 1, 13, 14 },
		{ CLOCK_TIME(17, 0) + 1, CLOCK_TIME(19, 0) + 1, 14, 15 },
		{ CLOCK_TIME(19, 0) + 1, CLOCK_TIME(24, 0) - 1, 15, 15 },
	},
	{
		{ CLOCK_TIME(0, 0), CLOCK_TIME(4, 0) + 1, 23, 23 },
		{ CLOCK_TIME(4, 0) + 1, CLOCK_TIME(6, 0), 23, 20 },
		{ CLOCK_TIME(6, 0), CLOCK_TIME(8, 0) + 1, 20, 21 },
		{ CLOCK_TIME(8, 0) + 1, CLOCK_TIME(16, 0), 21, 21 },
		{ CLOCK_TIME(16, 0), CLOCK_TIME(17, 0) + 1, 21, 22 },
		{ CLOCK_TIME(17, 0) + 1, CLOCK_TIME(19, 0) + 1, 22, 23 },
		{ CLOCK_TIME(19, 0) + 1, CLOCK_TIME(24, 0) - 1, 23, 23 },
	},
};

static float Environment_LerpWeight(uint16_t max, uint16_t min, uint16_t val) {
	float diff = max - min;
	float ret;
	
	if (diff != 0.0f) {
		ret = 1.0f - (max - val) / diff;
		
		if (!(ret >= 1.0f)) {
			return ret;
		}
	}
	
	return 1.0f;
}

// borrows from Environment_Update()
static EnvLightSettings GetEnvironment(struct Scene *scene, struct GuiInterop *gui)
{
	// quick gray background if no scene loaded
	if (!scene)
	{
		EnvLightSettings tmp;
		
		memset(&tmp, 0x80, sizeof(tmp));
		
		return tmp;
	}
	
	EnvLightSettings *lights = (void*)scene->headers[0].lights;
	
	if (gui->env.envPreviewMode == GUI_ENV_PREVIEW_EACH)
		return lights[gui->env.envPreviewEach];
		
	// testing
	// 0x8001 is noon
	static uint16_t skyboxTime = 0x8001; // gSaveContext.skyboxTime
	static uint16_t dayTime = 0x8001; // gSaveContext.dayTime
	//const int speed = 100;
	//skyboxTime += speed;
	//dayTime += speed;
	skyboxTime = gui->env.envPreviewTime;
	dayTime = skyboxTime;
	
	int envLightConfig = 0; // envCtx->lightConfig
	int envChangeLightNextConfig = 1; // envCtx->changeLightNextConfig
	TimeBasedLightEntry *configs = sTimeBasedLightConfigs[envLightConfig];
	EnvLightSettings *lightSettingsList = lights; // play->envCtx.lightSettingsList;
	EnvLightSettings dst; // envCtx->lightSettings;
	
	for (int i = 0; i < ARRAY_COUNT(*sTimeBasedLightConfigs); i++)
	{
		TimeBasedLightEntry config = configs[i];
		
		if (!((skyboxTime >= config.startTime)
				&& ((skyboxTime < config.endTime) || config.endTime == 0xFFFF)
		))
			continue;
		
		TimeBasedLightEntry changeToConfig = sTimeBasedLightConfigs[envChangeLightNextConfig][i];
		EnvLightSettings lightSetting = lightSettingsList[config.lightSetting];
		EnvLightSettings nextLightSetting = lightSettingsList[config.nextLightSetting];
		EnvLightSettings changeToNextLightSetting = lightSettingsList[changeToConfig.nextLightSetting];
		EnvLightSettings changeToLightSetting = lightSettingsList[changeToConfig.lightSetting];
		
		uint8_t blend8[2];
		int16_t blend16[2];
		
		float weight = Environment_LerpWeight(config.endTime, config.startTime, skyboxTime);
		float storms = 0;
		
		//sSandstormColorIndex = config.lightSetting & 3;
		//sNextSandstormColorIndex = config.nextLightSetting & 3;
		//sSandstormLerpScale = weight;
		
		// used for song of storms
		/*
		if (envCtx->changeLightEnabled) {
			storms = ((float)envCtx->changeDuration - envCtx->changeLightTimer) / envCtx->changeDuration;
			envCtx->changeLightTimer--;
			
			if (envCtx->changeLightTimer <= 0) {
				envCtx->changeLightEnabled = false;
				envLightConfig = envCtx->changeLightNextConfig;
			}
		}
		*/
		
		for (int j = 0; j < 3; j++)
		{
			// blend ambient color
			blend8[0] = LERP(lightSetting.ambientColor[j], nextLightSetting.ambientColor[j], weight);
			blend8[1] = LERP(changeToLightSetting.ambientColor[j], changeToNextLightSetting.ambientColor[j], weight);
			dst.ambientColor[j] = LERP(blend8[0], blend8[1], storms);
		}
		
		// set light1 direction for the sun
		dst.light1Dir[0] = -(SinS(dayTime - CLOCK_TIME(12, 0)) * 120.0f);
		dst.light1Dir[1] = CosS(dayTime - CLOCK_TIME(12, 0)) * 120.0f;
		dst.light1Dir[2] = CosS(dayTime - CLOCK_TIME(12, 0)) * 20.0f;
		
		// set light2 direction for the moon
		dst.light2Dir[0] = -dst.light1Dir[0];
		dst.light2Dir[1] = -dst.light1Dir[1];
		dst.light2Dir[2] = -dst.light1Dir[2];
		
		for (int j = 0; j < 3; j++)
		{
			// blend light1Color
			blend8[0] = LERP(lightSetting.light1Color[j], nextLightSetting.light1Color[j], weight);
			blend8[1] = LERP(changeToLightSetting.light1Color[j], changeToNextLightSetting.light1Color[j], weight);
			dst.light1Color[j] = LERP(blend8[0], blend8[1], storms);
			
			// blend light2Color
			blend8[0] = LERP(lightSetting.light2Color[j], nextLightSetting.light2Color[j], weight);
			blend8[1] = LERP(changeToLightSetting.light2Color[j], changeToNextLightSetting.light2Color[j], weight);
			dst.light2Color[j] = LERP(blend8[0], blend8[1], storms);
		}
		
		// blend fogColor
		for (int j = 0; j < 3; j++)
		{
			blend8[0] = LERP(lightSetting.fogColor[j], nextLightSetting.fogColor[j], weight);
			blend8[1] = LERP(changeToLightSetting.fogColor[j], changeToNextLightSetting.fogColor[j], weight);
			dst.fogColor[j] = LERP(blend8[0], blend8[1], storms);
		}
		
		blend16[0] = LERP16((lightSetting.fogNear & 0x3FF), (nextLightSetting.fogNear & 0x3FF), weight);
		blend16[1] = LERP16(changeToLightSetting.fogNear & 0x3FF, changeToNextLightSetting.fogNear & 0x3FF, weight);
		
		dst.fogNear = LERP16(blend16[0], blend16[1], storms);
		
		blend16[0] = LERP16(lightSetting.fogFar, nextLightSetting.fogFar, weight);
		blend16[1] = LERP16(changeToLightSetting.fogFar, changeToNextLightSetting.fogFar, weight);
		
		dst.fogFar = LERP16(blend16[0], blend16[1], storms);
		
		/*
		if (changeToConfig.nextLightSetting >= envCtx->numLightSettings)
		{
			// "The color palette setting seems to be wrong!"
			osSyncPrintf(VT_COL(RED, WHITE) "\nカラーパレットの設定がおかしいようです！" VT_RST);
			
			// "Palette setting = [] Last palette number = []"
			osSyncPrintf(VT_COL(RED, WHITE) "\n設定パレット＝[%d] 最後パレット番号＝[%d]\n" VT_RST,
						changeToConfig.nextLightSetting,
						envCtx->numLightSettings - 1);
		}
		*/
		
		break;
	}
	
	return dst;
}

#endif // day/night

// test implementation for sun movement
void DoLights(ZeldaLight *light)
{
	ZeldaVecS8 dirA = { UNFOLD_VEC3(light->diffuse_a_dir) };
	ZeldaVecS8 dirB = { UNFOLD_VEC3(light->diffuse_b_dir) };
	static int16_t dayTime = 0;
	dayTime += 100;
	
	//if (currentRoom && currentRoom->inDoorLights == false && lightCtx->curEnvId < 4)
	{
		switch (-1/*lightCtx->curEnvId*/) {
			case 0: {
				dayTime = 0x6000; // 06.00
				break;
			}
			case 1: {
				dayTime = 0x8001; // 12.00
				break;
			}
			case 2: {
				dayTime = 0xb556; // 17.00
				break;
			}
			case 3: {
				dayTime = 0xFFFF; // 24.00
				break;
			}
		}
		
		dirA = (ZeldaVecS8) {
			(SinS(dayTime) - 0x8000) * 120.0f,
			(CosS(dayTime) - 0x8000) * 120.0f,
			(CosS(dayTime) - 0x8000) * 20.0f
		};
		
		Vec3_Substract(dirB, (ZeldaVecS8) { 0 }, dirA);
	}
	
	n64_light_bind_dir(UNFOLD_VEC3(dirA), UNFOLD_RGB(light->diffuse_a));
	n64_light_bind_dir(UNFOLD_VEC3(dirB), UNFOLD_RGB(light->diffuse_b));
	n64_light_set_ambient(UNFOLD_RGB(light->ambient));
}

static struct Scene **gSceneP;

struct Scene *WindowOpenFile(void)
{
	const char *fn;
	
	assert(gSceneP);
	
	fn = noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, "zscene\0*.zscene\0", NULL, NULL);
	
	if (!fn)
		return *gSceneP;
	
	struct Scene *scene = WindowLoadScene(fn);
	
	return scene;
}

void WindowSaveScene(void)
{
	if (!*gSceneP)
		return;
	
	// overwrite loaded scene and rooms
	SceneToFilename(*gSceneP, 0);
	
	GuiPushModal("Saved scene and room files successfully.");
}

void WindowSaveSceneAs(void)
{
	if (!*gSceneP)
		return;
	
	const char *fn;
	
	fn = noc_file_dialog_open(NOC_FILE_DIALOG_SAVE, "zscene\0*.zscene\0", NULL, NULL);
	
	if (!fn)
		return;
	
	SceneToFilename(*gSceneP, fn);
	
	GuiPushModal("Saved scene and room files successfully.");
}

struct Scene *WindowLoadScene(const char *fn)
{
	LogDebug("%s", fn);
	struct Scene *scene = SceneFromFilenamePredictRooms(fn);
	
	// cleanup
	if (*gSceneP)
		SceneFree(*gSceneP);
	n64_clear_cache();
	
	// view new scene
	*gSceneP = scene;
	
	return scene;
}

struct Object *WindowLoadObject(const char *fn)
{
	struct Object *obj = ObjectFromFilename(fn);
	
	if (sb_count(obj->skeletons) && sb_count(obj->animations))
	{
		SkelAnime_Init(&gSkelAnimeTest, obj, &obj->skeletons[0], &obj->animations[0]);
		SkelAnime_Update(&gSkelAnimeTest, 0);
		//SkelAnime_Free(&gSkelAnimeTest);
		
		// get texture blobs
		// very WIP, just want to view texture blobs for now
		struct DataBlob *blobs = MiscSkeletonDataBlobs(obj->file, 0, obj->skeletons[0].segAddr);
		sb_array(struct TextureBlob, texBlobs) = 0;
		TextureBlobSbArrayFromDataBlobs(obj->file, blobs, &texBlobs);
		(*gSceneP)->textureBlobs = texBlobs;
	}
	
	return obj;
}

RayLine WindowGetRayLine(Vec2f point)
{
	int w = gState.winWidth;
	int h = gState.winHeight;
	Matrix *viewMtx = &gState.viewMtx;
	Matrix *projMtx = &gState.projMtx;
	
	Vec3f projA = Vec3f_New(point.x, point.y, 0.0f);
	Vec3f projB = Vec3f_New(point.x, point.y, 1.0f);
	Vec3f rayA, rayB;
	
	Matrix_Unproject(viewMtx, projMtx, &projA, &rayA, w, h);
	Matrix_Unproject(viewMtx, projMtx, &projB, &rayB, w, h);
	
	return RayLine_New(rayA, rayB);
}

RayLine WindowGetCursorRayLine(void)
{
	return WindowGetRayLine((Vec2f){ gState.input->mouse.pos.x, gState.input->mouse.pos.y });
}

void WindowClipPointIntoView(Vec3f* a, Vec3f normal)
{
	Vec4f test;
	f32 dot;
	struct CameraFly *camera = &gState.cameraFly;
	
	Matrix_MultVec3fToVec4f_Ext(a, &test, &gState.projViewMtx);
	
	if (test.w <= 0.0f)
	{
		dot = Vec3f_Dot(normal, Vec3f_LineSegDir(camera->eye, camera->lookAt));
		*a = Vec3f_Add(*a, Vec3f_MulVal(normal, -test.w / dot + 1.0f));
	}
}

Vec2f WindowGetLocalScreenPos(Vec3f point)
{
	f32 w = gState.winWidth * 0.5f;
	f32 h = gState.winHeight * 0.5f;
	Vec4f pos;
	
	Matrix_MultVec3fToVec4f_Ext(&point, &pos, &gState.projViewMtx);
	
	return Vec2f_New(
		w + (pos.x / pos.w) * w,
		h - (pos.y / pos.w) * h
	);
}

Vec4f WindowGetLocalScreenVec(Vec3f point)
{
	Vec4f pos;
	
	Matrix_MultVec3fToVec4f_Ext(&point, &pos, &gState.projViewMtx);
	
	return pos;
}

bool WindowTryInstance(bool requireSelectedInstance)
{
	return (!requireSelectedInstance || gGui->selectedInstance)
		&& !gState.hasCameraMoved
		&& GizmoIsIdle(gState.gizmo)
		&& gGui->instanceList
	;
}

bool WindowTryInstanceDuplicate(void)
{
	if (WindowTryInstance(true))
	{
		struct Instance newInst = *(gGui->selectedInstance);
		
		sb_push(*gGui->instanceList, newInst);
		gGui->selectedInstance = &sb_last(*gGui->instanceList);
		gGui->selectedInstance->prev = (typeof(gGui->selectedInstance->prev))INSTANCE_PREV_INIT;
		
		GuiPushModal("Duplicated instance.");
		GizmoSetupMove(gState.gizmo);
		
		return true;
	}
	
	return false;
}

bool WindowTryInstancePaste(bool showModal, bool deferWithRaycast)
{
	if (!gGui->clipboardHasInstance)
	{
		if (showModal)
			GuiPushModal("Clipboard is empty");
		
		return false;
	}
	
	if (deferWithRaycast)
	{
		gState.deferredInstancePaste = true;
		gGui->selectedInstance = 0;
		
		return false;
	}
	
	if (WindowTryInstance(false))
	{
		struct Instance newInst = gGui->clipboardInstance;
		
		newInst.pos = gGui->newSpawnPos;
		sb_push(*gGui->instanceList, newInst);
		gGui->selectedInstance = &sb_last(*gGui->instanceList);
		gGui->selectedInstance->prev = (typeof(gGui->selectedInstance->prev))INSTANCE_PREV_INIT;
		
		GuiPushModal("Pasted instance.");
		
		return true;
	}
	
	return false;
}

bool WindowTryInstanceDelete(bool showModal)
{
	if (WindowTryInstance(true))
	{
		int indexOf = sb_find_index(*gGui->instanceList, gGui->selectedInstance);
		
		if (indexOf >= 0)
		{
			LogDebug("delete instance %d", indexOf);
			sb_remove(*gGui->instanceList, indexOf);
			
			GizmoSetupIdle(gState.gizmo);
			GizmoRemoveChildren(gState.gizmo);
			gGui->selectedInstance = 0;
			
			if (showModal)
				GuiPushModal("Deleted instance");
		}
		
		return true;
	}
	
	return false;
}

bool WindowTryInstanceCut(bool showModal)
{
	if (WindowTryInstanceCopy(false)
		&& WindowTryInstanceDelete(false)
	)
	{
		if (showModal)
			GuiPushModal("Moved instance to clipboard");
		
		return true;
	}
	
	return false;
}

bool WindowTryInstanceCopy(bool showModal)
{
	if (WindowTryInstance(true))
	{
		gGui->clipboardHasInstance = true;
		gGui->clipboardInstance = *(gGui->selectedInstance);
		
		if (showModal)
			GuiPushModal("Copied instance to clipboard");
		
		return true;
	}
	
	return false;
}

GbiGfx* Gfx_TexScroll(u32 x, u32 y, s32 width, s32 height)
{
	GbiGfx* displayList = n64_graph_alloc(3 * sizeof(*displayList));
	
	x %= 512 << 2;
	y %= 512 << 2;
	
	gDPTileSync(displayList);
	gDPSetTileSize(displayList + 1, 0, x, y, x + ((width - 1) << 2), y + ((height - 1) << 2));
	gSPEndDisplayList(displayList + 2);

	return displayList;
}

GbiGfx* Gfx_TwoTexScroll(s32 tile1, u32 x1, u32 y1, s32 width1, s32 height1, s32 tile2, u32 x2, u32 y2, s32 width2, s32 height2)
{
	GbiGfx* displayList = n64_graph_alloc(5 * sizeof(*displayList));
	
	x1 %= 512 << 2;
	y1 %= 512 << 2;
	x2 %= 512 << 2;
	y2 %= 512 << 2;
	
	gDPTileSync(displayList);
	gDPSetTileSize(displayList + 1, tile1, x1, y1, x1 + ((width1 - 1) << 2), y1 + ((height1 - 1) << 2));
	gDPTileSync(displayList + 2);
	gDPSetTileSize(displayList + 3, tile2, x2, y2, x2 + ((width2 - 1) << 2), y2 + ((height2 - 1) << 2));
	gSPEndDisplayList(displayList + 4);

	return displayList;
}

static void AnimateTheWater(void)
{
	static float sGameplayFrames = 0;
	int32_t gameplayFrames = sGameplayFrames;
	sGameplayFrames += 0.1; // supports scrolling in reverse as well
	
	// pond
	gSPSegment(POLY_XLU_DISP++, 0x09,
		Gfx_TwoTexScroll(G_TX_RENDERTILE, 127 - gameplayFrames % 128,
			(gameplayFrames * 1) % 128, 32, 32, 1, gameplayFrames % 128, (gameplayFrames * 1) % 128,
			32, 32
		)
	);
	// waterfall
	gSPSegment(POLY_XLU_DISP++, 0x08,
		Gfx_TwoTexScroll(G_TX_RENDERTILE, 127 - gameplayFrames % 128,
			(gameplayFrames * 10) % 128, 32, 32, 1, gameplayFrames % 128,
			(gameplayFrames * 10) % 128, 32, 32
		)
	);
	
	// this controls the blending on the multi textures
	// grass
	gDPPipeSync(POLY_OPA_DISP++);
	gDPSetEnvColor(POLY_OPA_DISP++, 128, 128, 128, 128);
	// water
	gDPPipeSync(POLY_XLU_DISP++);
	gDPSetEnvColor(POLY_XLU_DISP++, 128, 128, 128, 128);
}

// Multiplies a 4 component row vector [ src , 1 ] by the matrix mf and writes the resulting xyz components to dest.
static void SkinMatrix_Vec3fMtxFMultXYZ(Matrix* mf, Vec3f* src, Vec3f* dest)
{
	float mx = mf->xx;
	float my = mf->xy;
	float mz = mf->xz;
	float mw = mf->xw;

	dest->x = mw + ((src->x * mx) + (src->y * my) + (src->z * mz));

	mx = mf->yx;
	my = mf->yy;
	mz = mf->yz;
	mw = mf->yw;
	dest->y = mw + ((src->x * mx) + (src->y * my) + (src->z * mz));

	mx = mf->zx;
	my = mf->zy;
	mz = mf->zz;
	mw = mf->zw;
	dest->z = mw + ((src->x * mx) + (src->y * my) + (src->z * mz));
}

// FIXME using distance-to-camera as an approximation for now, but is imperfect
//       (see the code that has been commented out for how it is intended to work)
static void DrawRoomCullable(struct RoomHeader *header, int16_t zFar, uint32_t flags)
{
	typedef struct RoomShapeCullableEntryLinked
	{
		struct RoomShapeCullableEntryLinked *prev;
		struct RoomShapeCullableEntryLinked *next;
		const struct RoomMeshSimple *entry;
		float boundsNearZ;
	} RoomShapeCullableEntryLinked;
	
	#define ROOM_SHAPE_CULLABLE_MAX_ENTRIES 128
	#define ROOM_DRAW_OPA 0b01
	#define ROOM_DRAW_XLU 0b10
	#define ABS_ALT(x) ((x) < 0 ? -(x) : (x))
	
	static RoomShapeCullableEntryLinked linkedEntriesBuffer[ROOM_SHAPE_CULLABLE_MAX_ENTRIES];
	RoomShapeCullableEntryLinked* head = NULL;
	RoomShapeCullableEntryLinked* tail = NULL;
	RoomShapeCullableEntryLinked* iter;
	RoomShapeCullableEntryLinked* insert = linkedEntriesBuffer;
	float entryBoundsNearZ;

	Matrix viewProjectionMtxF = gState.projViewMtx;
	Vec3f projectionMtxFDiagonal = {
		gState.projMtx.xx, gState.projMtx.yy, -gState.projMtx.zz
	};

	// Pick and sort entries by depth
	const struct RoomMeshSimple *cullable = header->displayLists;
	const int numEntries = sb_count(header->displayLists);
	for (int i = 0; i < numEntries; i++, cullable++)
	{
		// Project the entry position, to get the depth it is at.
		Vec3f pos = { UNFOLD_VEC3_EXT(cullable->center, * 1) };
		Vec3f projectedPos;
		
		SkinMatrix_Vec3fMtxFMultXYZ(&viewProjectionMtxF, &pos, &projectedPos);
		// testing alternate method
		//Vec3f step2;
		//SkinMatrix_Vec3fMtxFMultXYZ(&gState.viewMtx, &pos, &step2);
		//SkinMatrix_Vec3fMtxFMultXYZ(&gState.projMtx, &step2, &projectedPos);
		
		projectedPos.z *= 1.0f / projectionMtxFDiagonal.z; // original math
		
		//Vec4f test = WindowGetLocalScreenVec(pos);
		//LogDebug("test %f %f", test.z, test.w);
		//LogDebug(" -> %f", projectedPos.z);
		
		float var_fv1 = ABS_ALT(cullable->radius);
		
		/*
		if (cullable->xlu == 0x03004CD0 // back dl
			|| cullable->xlu == 0x03004760 // front dl
		)
		{
			LogDebug("%08x:", cullable->xlu);
			LogDebug(" { %.f %.f %.f }", projectedPos.x, projectedPos.y, projectedPos.z);
			LogDebug(" { %.f %.f %.f }", pos.x, pos.y, pos.z);
			LogDebug(" %.f -> %.f", projectedPos.z, projectedPos.z - var_fv1);
			LogDebug(" recip %f", 1.0f / projectionMtxFDiagonal.z);
			
			float testA = projectedPos.z - var_fv1;
			float testB = Vec3f_DistXYZ(pos, gState.cameraFly.eye); // also try lookAt
			LogDebug(" %.f vs %.f ", testA, testB);
		}
		*/
		
		// If the entry bounding sphere isn't fully before the rendered depth range
		if (-var_fv1 < projectedPos.z)
		{
			// Compute the depth of the nearest point in the entry's bounding sphere
			entryBoundsNearZ = projectedPos.z - var_fv1;
			entryBoundsNearZ = Vec3f_DistXYZ(pos, gState.cameraFly.lookAt); // hack
			
			// If the entry bounding sphere isn't fully beyond the rendered depth range
			if (entryBoundsNearZ < zFar)
			{
				// This entry will be rendered
				insert->entry = cullable;

				if (cullable->radius < 0)
					insert->boundsNearZ = FLT_MAX;
				else
					insert->boundsNearZ = entryBoundsNearZ;

				// Insert into the linked list, ordered by ascending
				// depth of the nearest point in the bounding sphere
				iter = head;
				if (iter == NULL)
				{
					head = tail = insert;
					insert->prev = insert->next = NULL;
				}
				else
				{
					do
					{
						if (insert->boundsNearZ < iter->boundsNearZ)
							break;
						
						iter = iter->next;
					} while (iter != NULL);

					if (iter == NULL)
					{
						insert->prev = tail;
						insert->next = NULL;
						tail->next = insert;
						tail = insert;
					}
					else
					{
						insert->prev = iter->prev;
						if (insert->prev == NULL)
							head = insert;
						else
							insert->prev->next = insert;
						
						iter->prev = insert;
						insert->next = iter;
					}
				}

				insert++;
			}
		}
	}
	
	//LogDebug("draw %d / %d", (int)(insert - linkedEntriesBuffer), numEntries);

	//! FAKE: Similar trick used in OoT
	//R_ROOM_CULL_NUM_ENTRIES = numEntries & 0xFFFF & 0xFFFF & 0xFFFF;

	// Draw entries, from nearest to furthest

	if (flags & ROOM_DRAW_OPA)
	{
		for (int i = 1; head != NULL; head = head->next, i++)
		{
			cullable = head->entry;
			
			uint32_t displayList = cullable->opa;
			
			/*
			// Debug mode drawing
			if (R_ROOM_CULL_DEBUG_MODE != ROOM_CULL_DEBUG_MODE_OFF)
			{
				if (((R_ROOM_CULL_DEBUG_MODE == ROOM_CULL_DEBUG_MODE_UP_TO_TARGET) &&
					(i <= R_ROOM_CULL_DEBUG_TARGET)) ||
					((R_ROOM_CULL_DEBUG_MODE == ROOM_CULL_DEBUG_MODE_ONLY_TARGET) &&
					(i == R_ROOM_CULL_DEBUG_TARGET))
				)
				{
					if (displayList)
					{
						gSPDisplayList(POLY_OPA_DISP++, displayList);
					}
				}
			}
			else
			*/
			{
				if (displayList)
				{
					gSPDisplayList(POLY_OPA_DISP++, displayList);
				}
			}
		}
	}
	
	if (flags & ROOM_DRAW_XLU)
	{
		for (; tail != NULL; tail = tail->prev)
		{
			cullable = tail->entry;
			
			uint32_t displayList = cullable->xlu;
			
			if (displayList)
			{
				// TODO opacity based on distance
				//      (need to resolve iREG values)
				/*
				if (cullable->radius & 1)
				{
					float temp_fv0 = tail->boundsNearZ - (float)(iREG(93) + 0xBB8);
					float temp_fv1 = iREG(94) + 0x7D0;

					if (temp_fv0 < temp_fv1)
					{
						int xluOpacity;
						
						if (temp_fv0 < 0.0f)
							xluOpacity = 255;
						else
							xluOpacity = 255 - (int32_t)((temp_fv0 / temp_fv1) * 255.0f);
						
						gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, xluOpacity);
						gSPDisplayList(POLY_XLU_DISP++, displayList);
					}
				}
				else
				*/
				{
					gSPDisplayList(POLY_XLU_DISP++, displayList);
				}
			}
		}
	}
}

static void RaycastInstanceList(sb_array(struct Instance, *instanceList), struct Gizmo *gizmo, RayLine ray)
{
	struct Instance *instances;
	
	if (!instanceList)
		return;
	
	if (!(instances = *instanceList))
		return;
	
	sb_foreach(instances, {
		if (Col3D_LineVsSphere(
			&ray
			, &(Sphere){
				.pos = each->pos
				, .r = 20
			}
			, 0
		)) {
			// TODO construct a list of collisions and choose the nearest instance
			// TODO if multiple instances overlap, each click should cycle through them
			GizmoSetPosition(gizmo, UNFOLD_VEC3(each->pos));
			GizmoAddChild(gizmo, &each->pos);
			gGui->selectedInstance = each;
			break;
		}
	});
}

struct ActorRenderCode *gRenderCodeHandle = 0;

static void RenderCodeWriteFn(WrenVM* vm, const char* text)
{
	// skip messages consisting only of whitespace
	if (strspn(text, "\r\n\t ") == strlen(text))
		return;
	
	LogDebug("%s", text);
}

static void RenderCodeErrorFn(WrenVM* vm, WrenErrorType errorType,
	const char* module, int line,
	const char* msg
)
{
	line += gRenderCodeHandle->lineErrorOffset;
	
	switch (errorType)
	{
		case WREN_ERROR_COMPILE:
			LogDebug("[%s line %d] [Error] %s", module, line, msg);
			break;
		case WREN_ERROR_STACK_TRACE:
			LogDebug("[%s line %d] in %s", module, line, msg);
			break;
		case WREN_ERROR_RUNTIME:
			LogDebug("[Runtime Error] %s", msg);
			break;
	}
}

static bool gRenderCodeDrewSomething;
static double sXrotGlobal = 0;
static double sYrotGlobal = 0;
static double sZrotGlobal = 0;
static Vec3f sPosGlobal = {0};
static GbiGfx **sRenderCodeSegment = 0;
static void *gRenderCodeBillboards = 0;
static Matrix gBillboardMatrix[2];
static WrenForeignMethodFn RenderCodeBindForeignMethod(
	WrenVM* vm
	, const char* module
	, const char* className
	, bool isStatic
	, const char* signature
)
{
	static double sXscale = 1;
	static double sYscale = 1;
	static double sZscale = 1;
	static double sXposLocal = 0;
	static double sYposLocal = 0;
	static double sZposLocal = 0;
	static struct Object *sObject = 0;
	
	void ReadyMatrix(struct Instance *inst, bool shouldPop, bool shouldUse) {
		Matrix_Push(); {
			Matrix_Translate(UNFOLD_VEC3(sPosGlobal), MTXMODE_NEW);
			Matrix_RotateY_s(sYrotGlobal, MTXMODE_APPLY);
			Matrix_RotateX_s(sXrotGlobal, MTXMODE_APPLY);
			Matrix_RotateZ_s(sZrotGlobal, MTXMODE_APPLY);
			Matrix_Scale(sXscale, sYscale, sZscale, MTXMODE_APPLY);
			Matrix_Translate(sXposLocal, sYposLocal, sZposLocal, MTXMODE_APPLY);
			
			if (shouldUse) {
				gSPMatrix((*sRenderCodeSegment)++, Matrix_NewMtxN64(),
					G_MTX_MODELVIEW | G_MTX_LOAD
				);
			}
		} if (shouldPop) Matrix_Pop();
	}
	
#define WREN_UDATA ((struct Instance*)wrenGetUserData(vm))
#define streq(A, B) !strcmp(A, B)
	void InstGetXpos(WrenVM* vm) { wrenSetSlotDouble(vm, 0, WREN_UDATA->pos.x); }
	void InstGetYpos(WrenVM* vm) { wrenSetSlotDouble(vm, 0, WREN_UDATA->pos.y); }
	void InstGetZpos(WrenVM* vm) { wrenSetSlotDouble(vm, 0, WREN_UDATA->pos.z); }
	void InstGetXrot(WrenVM* vm) { wrenSetSlotDouble(vm, 0, WREN_UDATA->xrot); }
	void InstGetYrot(WrenVM* vm) { wrenSetSlotDouble(vm, 0, WREN_UDATA->yrot); }
	void InstGetZrot(WrenVM* vm) { wrenSetSlotDouble(vm, 0, WREN_UDATA->zrot); }
	void InstGetUuid(WrenVM* vm) { wrenSetSlotDouble(vm, 0, WREN_UDATA->prev.uuid); }
	void InstGetSnapAngleX(WrenVM* vm) { wrenSetSlotDouble(vm, 0, WREN_UDATA->snapAngle.x); }
	void InstGetSnapAngleY(WrenVM* vm) { wrenSetSlotDouble(vm, 0, WREN_UDATA->snapAngle.y); }
	void InstGetSnapAngleZ(WrenVM* vm) { wrenSetSlotDouble(vm, 0, WREN_UDATA->snapAngle.z); }
	void InstGetPositionChanged(WrenVM* vm) {
		struct Instance *inst = WREN_UDATA;
		const float threshold = 0.01f;
		Vec3f posPrev = inst->prev.pos;
		wrenSetSlotBool(vm, 0,
			fabs(inst->pos.x - posPrev.x) > threshold
			|| fabs(inst->pos.y - posPrev.y) > threshold
			|| fabs(inst->pos.z - posPrev.z) > threshold
		);
		inst->prev.pos = inst->pos;
	}
	void InstGetPropertyChanged(WrenVM* vm) {
		struct Instance *inst = WREN_UDATA;
		wrenSetSlotBool(vm, 0,
			inst->params != inst->prev.params
			|| inst->xrot != inst->prev.xrot
			|| inst->yrot != inst->prev.yrot
			|| inst->zrot != inst->prev.zrot
			|| inst->id != inst->prev.id
		);
		inst->prev.params = inst->params;
		inst->prev.xrot = inst->xrot;
		inst->prev.yrot = inst->yrot;
		inst->prev.zrot = inst->zrot;
	}
	void InstGetPositionSnapped(WrenVM* vm) {
		struct Instance *inst = WREN_UDATA;
		wrenSetSlotBool(vm, 0, inst->prev.positionSnapped);
		inst->prev.positionSnapped = false;
	}
	void InstSetRotation(WrenVM* vm) {
		struct Instance *inst = WREN_UDATA;
		inst->xrot = wrenGetSlotDouble(vm, 1);
		inst->yrot = wrenGetSlotDouble(vm, 2);
		inst->zrot = wrenGetSlotDouble(vm, 3);
	}
	#define InstSetFaceSnapVector(INST, VEC) \
		Vec3f A = VEC; \
		Vec3f *B = &INST->faceSnapVector; \
		if (A.x != B->x || A.y != B->y || A.z != B->z) \
			INST->prev.triNormal16 = (Vec3s){0, 0, 0}; \
		*B = A;
	void InstSetFaceSnapVector0(WrenVM *vm) {
		InstSetFaceSnapVector(WREN_UDATA, ((Vec3f){0, 1, 0}));
	}
	void InstSetFaceSnapVector3(WrenVM *vm) {
		InstSetFaceSnapVector(WREN_UDATA, ((Vec3f) {
			wrenGetSlotDouble(vm, 1),
			wrenGetSlotDouble(vm, 2),
			wrenGetSlotDouble(vm, 3),
		}));
	}
	void InstClearFaceSnapVector(WrenVM *vm) {
		InstSetFaceSnapVector(WREN_UDATA, ((Vec3f){0, 0, 0}));
	}
	
	void DrawSetScale3(WrenVM* vm) {
		sXscale = wrenGetSlotDouble(vm, 1);
		sYscale = wrenGetSlotDouble(vm, 2);
		sZscale = wrenGetSlotDouble(vm, 3);
		//LogDebug("DrawSetScale3 = %f %f %f", sXscale, sYscale, sZscale);
	}
	void DrawSetScale1(WrenVM* vm) {
		sXscale = wrenGetSlotDouble(vm, 1);
		sYscale = sXscale;
		sZscale = sXscale;
		//LogDebug("DrawSetScale1 = %f %f %f", sXscale, sYscale, sZscale);
	}
	void DrawUseObjectSlot(WrenVM* vm) {
		int slot = wrenGetSlotDouble(vm, 1);
		int objectId = GuiGetActorObjectIdFromSlot(WREN_UDATA->id, slot);
		sObject = GuiGetObjectDataFromId(objectId);
		if (sObject) {
			const void *data = sObject->file->data;
			int segment =
				objectId == 0x0001
					? 0x04
					: objectId <= 0x0003
						? 0x05
						: 0x06
			;
			// TODO also allow objects to specify their own segments
			gSPSegment(POLY_OPA_DISP++, segment, data);
			gSPSegment(POLY_XLU_DISP++, segment, data);
		}
	}
	void DrawSetLocalPosition(WrenVM* vm) {
		sXposLocal = wrenGetSlotDouble(vm, 1);
		sYposLocal = wrenGetSlotDouble(vm, 2);
		sZposLocal = wrenGetSlotDouble(vm, 3);
	}
	void DrawSetGlobalPosition(WrenVM* vm) {
		sPosGlobal = (Vec3f) {
			wrenGetSlotDouble(vm, 1),
			wrenGetSlotDouble(vm, 2),
			wrenGetSlotDouble(vm, 3),
		};
	}
	void DrawSetGlobalRotation(WrenVM* vm) {
		sXrotGlobal = wrenGetSlotDouble(vm, 1);
		sYrotGlobal = wrenGetSlotDouble(vm, 2);
		sZrotGlobal = wrenGetSlotDouble(vm, 3);
	}
	void DrawMesh(WrenVM* vm) {
		uint32_t address = wrenGetSlotDouble(vm, 1);
		struct Instance *inst = WREN_UDATA;
		//LogDebug("address = %08x", address);
		ReadyMatrix(inst, true, true);
		gSPDisplayList((*sRenderCodeSegment)++, address);
		gRenderCodeDrewSomething = true;
	}
	void DrawSetPrimColor3(WrenVM* vm) {
		gDPSetPrimColor((*sRenderCodeSegment)++,
			0, // minlevel
			0, // lodfrac
			wrenGetSlotDouble(vm, 1), // r
			wrenGetSlotDouble(vm, 2), // g
			wrenGetSlotDouble(vm, 3), // b
			255 // a
		);
	}
	void DrawSetPrimColor4(WrenVM* vm) {
		gDPSetPrimColor((*sRenderCodeSegment)++,
			0, // minlevel
			0, // lodfrac
			wrenGetSlotDouble(vm, 1), // r
			wrenGetSlotDouble(vm, 2), // g
			wrenGetSlotDouble(vm, 3), // b
			wrenGetSlotDouble(vm, 4) // a
		);
	}
	void DrawSetPrimColor6(WrenVM* vm) {
		gDPSetPrimColor((*sRenderCodeSegment)++,
			wrenGetSlotDouble(vm, 1), // minlevel
			wrenGetSlotDouble(vm, 2), // lodfrac
			wrenGetSlotDouble(vm, 3), // r
			wrenGetSlotDouble(vm, 4), // g
			wrenGetSlotDouble(vm, 5), // b
			wrenGetSlotDouble(vm, 6) // a
		);
	}
	void DrawSetEnvColor3(WrenVM* vm) {
		gDPSetEnvColor((*sRenderCodeSegment)++,
			wrenGetSlotDouble(vm, 1), // r
			wrenGetSlotDouble(vm, 2), // g
			wrenGetSlotDouble(vm, 3), // b
			255 // a
		);
	}
	void DrawSetEnvColor4(WrenVM* vm) {
		gDPSetEnvColor((*sRenderCodeSegment)++,
			wrenGetSlotDouble(vm, 1), // r
			wrenGetSlotDouble(vm, 2), // g
			wrenGetSlotDouble(vm, 3), // b
			wrenGetSlotDouble(vm, 4) // a
		);
	}
	void DrawBeginSegment(WrenVM *vm) {
		static GbiGfx *result = 0;
		result = n64_graph_alloc(0x100);
		sRenderCodeSegment = &result;
		int segmentNumber = wrenGetSlotDouble(vm, 1);
		gSPSegment(POLY_OPA_DISP++, segmentNumber, result);
		gSPSegment(POLY_XLU_DISP++, segmentNumber, result);
	}
	void DrawEndSegment(WrenVM *vm) {
		gSPEndDisplayList(*sRenderCodeSegment);
		sRenderCodeSegment = &POLY_OPA_DISP;
	}
	void DrawMatrix(WrenVM *vm) {
		uint32_t indexOrAddress = wrenGetSlotDouble(vm, 1);
		// is index, rather than segment address
		if (indexOrAddress < 0x01000000) {
			// TODO referencing matrices by index rather than by address
		}
		//LogDebug("draw matrix %08x", indexOrAddress);
		gSPMatrix((*sRenderCodeSegment)++, indexOrAddress, G_MTX_MODELVIEW | G_MTX_LOAD);
	}
	void DrawMatrixNewFromBillboardSphere(WrenVM *vm) {
		struct Instance *inst = WREN_UDATA;
		ReadyMatrix(inst, false, false);
		Matrix_ReplaceRotation(&gBillboardMatrix[0]);
		gSPMatrix((*sRenderCodeSegment)++, Matrix_NewMtxN64(),
			G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW
		);
		Matrix_Pop();
		//wrenSetSlotDouble(vm, 0, 0x01000000);
	}
	void DrawMatrixNewFromBillboardCylinder(WrenVM *vm) {
		struct Instance *inst = WREN_UDATA;
		ReadyMatrix(inst, false, false);
		Matrix_ReplaceRotation(&gBillboardMatrix[1]);
		gSPMatrix((*sRenderCodeSegment)++, Matrix_NewMtxN64(),
			G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW
		);
		Matrix_Pop();
		//wrenSetSlotDouble(vm, 0, 0x01000040);
	}
	void DrawTwoTexScroll8(WrenVM *vm) {
		void *result = Gfx_TwoTexScroll(
			G_TX_RENDERTILE,
			wrenGetSlotDouble(vm, 1),
			wrenGetSlotDouble(vm, 2),
			wrenGetSlotDouble(vm, 3),
			wrenGetSlotDouble(vm, 4),
			1,
			wrenGetSlotDouble(vm, 5),
			wrenGetSlotDouble(vm, 6),
			wrenGetSlotDouble(vm, 7),
			wrenGetSlotDouble(vm, 8)
		);
		gSPDisplayList((*sRenderCodeSegment)++, result);
	}
	void DrawTwoTexScroll10(WrenVM *vm) {
		void *result = Gfx_TwoTexScroll(
			wrenGetSlotDouble(vm, 1),
			wrenGetSlotDouble(vm, 2),
			wrenGetSlotDouble(vm, 3),
			wrenGetSlotDouble(vm, 4),
			wrenGetSlotDouble(vm, 5),
			wrenGetSlotDouble(vm, 6),
			wrenGetSlotDouble(vm, 7),
			wrenGetSlotDouble(vm, 8),
			wrenGetSlotDouble(vm, 9),
			wrenGetSlotDouble(vm, 10)
		);
		gSPDisplayList((*sRenderCodeSegment)++, result);
	}
	void DrawPopulateSegment2(WrenVM *vm) {
		if (sObject) {
			int segment = wrenGetSlotDouble(vm, 1);
			uint32_t address = wrenGetSlotDouble(vm, 2);
			const uint8_t *data = sObject->file->data;
			data += address & 0x00ffffff;
			gSPSegment(POLY_OPA_DISP++, segment, data);
			gSPSegment(POLY_XLU_DISP++, segment, data);
		}
	}
	void DrawPopulateSegment3(WrenVM *vm) {
		int slot = wrenGetSlotDouble(vm, 1);
		int objectId = GuiGetActorObjectIdFromSlot(WREN_UDATA->id, slot);
		struct Object *obj = GuiGetObjectDataFromId(objectId);
		if (obj) {
			int segment = wrenGetSlotDouble(vm, 1);
			uint32_t address = wrenGetSlotDouble(vm, 2);
			const uint8_t *data = obj->file->data;
			data += address & 0x00ffffff;
			gSPSegment(POLY_OPA_DISP++, segment, data);
			gSPSegment(POLY_XLU_DISP++, segment, data);
		}
	}
	void MathSinS(WrenVM* vm) {
		double v = wrenGetSlotDouble(vm, 1);
		wrenSetSlotDouble(vm, 0, sins(v) * (1.0 / 32767.0));
	}
	void MathCosS(WrenVM* vm) {
		double v = wrenGetSlotDouble(vm, 1);
		wrenSetSlotDouble(vm, 0, coss(v) * (1.0 / 32767.0));
	}
	void MathDegToBin(WrenVM* vm) {
		double v = wrenGetSlotDouble(vm, 1);
		wrenSetSlotDouble(vm, 0, DegToBin(v));
	}
	void GlobalGameplayFrames(WrenVM* vm) {
		wrenSetSlotDouble(vm, 0, sGameplayFrames);
	}
	void CollisionRaycastSnapToFloor(WrenVM *vm) {
		Vec3f point = {
			wrenGetSlotDouble(vm, 1),
			wrenGetSlotDouble(vm, 2),
			wrenGetSlotDouble(vm, 3)
		};
		struct Scene *scene = *gSceneP;
		CollisionHeader *collision = &scene->collisions[0];
		wrenSetSlotDouble(vm, 0,
			Col3D_SnapToFloor(
				point,
				collision->vtxList,
				collision->numVertices,
				collision->triListMasked,
				collision->numPolygons
			)
		);
	}
	
	void DrawSkeleton(WrenVM* vm) {
		struct Instance *inst = WREN_UDATA;
		ReadyMatrix(inst, false, true);
		if (sObject) {
			if (inst->skelanime.limbCount == 0
				&& sb_count(sObject->skeletons)
				&& sb_count(sObject->animations)
			) {
				uint32_t address = wrenGetSlotDouble(vm, 1);
				// is index, rather than segment address
				if (address < 0x01000000) {
					SkelAnime_Init(
						&inst->skelanime
						, sObject
						, &sObject->skeletons[address]
						, &sObject->animations[0]
					);
					SkelAnime_Update(&inst->skelanime, 0);
				}
				// TODO address based skelanime
			}
			if (inst->skelanime.limbCount) {
				SkelAnime_Update(&inst->skelanime, gInput.delta_time_sec * (20.0));
				SkelAnime_Draw(&inst->skelanime, SKELANIME_TYPE_FLEX);
				gRenderCodeDrewSomething = true;
			}
		}
		Matrix_Pop();
	}
	
	if (streq(module, "main")) {
		// no setters, are read-only
		if (streq(className, "Inst")) {
			if (streq(signature, "Xpos")) return InstGetXpos;
			else if (streq(signature, "Ypos")) return InstGetYpos;
			else if (streq(signature, "Zpos")) return InstGetZpos;
			else if (streq(signature, "Xrot")) return InstGetXrot;
			else if (streq(signature, "Yrot")) return InstGetYrot;
			else if (streq(signature, "Zrot")) return InstGetZrot;
			else if (streq(signature, "Uuid")) return InstGetUuid;
			else if (streq(signature, "SnapAngleX")) return InstGetSnapAngleX;
			else if (streq(signature, "SnapAngleY")) return InstGetSnapAngleY;
			else if (streq(signature, "SnapAngleZ")) return InstGetSnapAngleZ;
			else if (streq(signature, "PositionChanged")) return InstGetPositionChanged;
			else if (streq(signature, "PropertyChanged")) return InstGetPropertyChanged;
			else if (streq(signature, "PositionSnapped")) return InstGetPositionSnapped;
			else if (streq(signature, "SetRotation(_,_,_)")) return InstSetRotation;
			else if (streq(signature, "SetFaceSnapVector()")) return InstSetFaceSnapVector0;
			else if (streq(signature, "SetFaceSnapVector(_,_,_)")) return InstSetFaceSnapVector3;
			else if (streq(signature, "ClearFaceSnapVector()")) return InstClearFaceSnapVector;
			//else if (streq(signature, "Xpos=(_)")) return InstSetXpos; // no setter, is read-only
		}
		else if (streq(className, "Draw")) {
			if (streq(signature, "SetScale(_,_,_)")) return DrawSetScale3;
			else if (streq(signature, "SetScale(_)")) return DrawSetScale1;
			else if (streq(signature, "UseObjectSlot(_)")) return DrawUseObjectSlot;
			else if (streq(signature, "Mesh(_)")) return DrawMesh;
			else if (streq(signature, "Skeleton(_)")) return DrawSkeleton;
			else if (streq(signature, "SetLocalPosition(_,_,_)")) return DrawSetLocalPosition;
			else if (streq(signature, "SetGlobalPosition(_,_,_)")) return DrawSetGlobalPosition;
			else if (streq(signature, "SetGlobalRotation(_,_,_)")) return DrawSetGlobalRotation;
			else if (streq(signature, "SetPrimColor(_,_,_)")) return DrawSetPrimColor3;
			else if (streq(signature, "SetPrimColor(_,_,_,_)")) return DrawSetPrimColor4;
			else if (streq(signature, "SetPrimColor(_,_,_,_,_,_)")) return DrawSetPrimColor6;
			else if (streq(signature, "SetEnvColor(_,_,_)")) return DrawSetEnvColor3;
			else if (streq(signature, "SetEnvColor(_,_,_,_)")) return DrawSetEnvColor4;
			
			else if (streq(signature, "BeginSegment(_)")) return DrawBeginSegment;
			else if (streq(signature, "EndSegment()")) return DrawEndSegment;
			else if (streq(signature, "Matrix(_)")) return DrawMatrix;
			else if (streq(signature, "MatrixNewFromBillboardSphere()")) return DrawMatrixNewFromBillboardSphere;
			else if (streq(signature, "MatrixNewFromBillboardCylinder()")) return DrawMatrixNewFromBillboardCylinder;
			else if (streq(signature, "TwoTexScroll(_,_,_,_,_,_,_,_)")) return DrawTwoTexScroll8;
			else if (streq(signature, "TwoTexScroll(_,_,_,_,_,_,_,_,_,_)")) return DrawTwoTexScroll10;
			else if (streq(signature, "PopulateSegment(_,_)")) return DrawPopulateSegment2;
			else if (streq(signature, "PopulateSegment(_,_,_)")) return DrawPopulateSegment3;
		}
		else if (streq(className, "Math")) {
			if (streq(signature, "SinS(_)")) return MathSinS;
			else if (streq(signature, "CosS(_)")) return MathCosS;
			else if (streq(signature, "DegToBin(_)")) return MathDegToBin;
		}
		else if (streq(className, "Collision")) {
			if (streq(signature, "RaycastSnapToFloor(_,_,_)")) return CollisionRaycastSnapToFloor;
		}
		else if (streq(className, "Global")) {
			if (streq(signature, "GameplayFrames")) return GlobalGameplayFrames;
		}
	}
	
	return 0;
}

// returns true if it drew geometry
bool RenderCodeGo(struct Instance *inst)
{
	struct ActorRenderCode *rc = GuiGetActorRenderCode(inst->id);
	
	if (!rc)
		return false;
	
	gRenderCodeDrewSomething = false;
	
	// run vm
	if (rc->type == ACTOR_RENDER_CODE_TYPE_VM)
	{
		GuiApplyActorRenderCodeProperties(inst);
		
		WrenVM *vm = rc->vm;
		wrenSetUserData(vm, inst);
		
		wrenEnsureSlots(vm, 1);
		wrenSetSlotHandle(vm, 0, rc->slotHandle);
		
		n64_buffer_clear();
		n64_draw_dlist(gfxDisableXray);
		sXrotGlobal = inst->xrot;
		sYrotGlobal = inst->yrot;
		sZrotGlobal = inst->zrot;
		sPosGlobal = inst->pos;
		// billboard matrices live in segment 0x01
		gSPSegment(POLY_OPA_DISP++, 0x1, gRenderCodeBillboards);
		gSPSegment(POLY_XLU_DISP++, 0x1, gRenderCodeBillboards);
		sRenderCodeSegment = &POLY_OPA_DISP;
		if (wrenCall(vm, rc->callHandle) != WREN_RESULT_SUCCESS)
			LogDebug("failed to invoke function");
		n64_buffer_flush(false);
		n64_draw_dlist(gfxEnableXray);
	}
	// compile source into vm
	else if (rc->type == ACTOR_RENDER_CODE_TYPE_SOURCE)
	{
		const char *module = "main";
		gRenderCodeHandle = rc;
		WrenConfiguration config;
		wrenInitConfiguration(&config);
			config.writeFn = &RenderCodeWriteFn;
			config.errorFn = &RenderCodeErrorFn;
			config.bindForeignMethodFn = &RenderCodeBindForeignMethod;
		WrenVM* vm = wrenNewVM(&config);
		
		WrenInterpretResult result = wrenInterpret(vm, module, rc->src);
		
		//LogDebug("src\n\n\n %s \n\n\n", rc->src);
		
		// success
		if (result == WREN_RESULT_SUCCESS)
		{
			rc->type = ACTOR_RENDER_CODE_TYPE_VM;
			rc->vm = vm;
			
			// get handles
			wrenEnsureSlots(vm, 1);
			wrenGetVariable(vm, module, "hooks", 0);
			
			rc->slotHandle = wrenGetSlotHandle(vm, 0);
			rc->callHandle = wrenMakeCallHandle(vm, "draw()");
			
			sb_push(rc->vmHandles, rc->slotHandle);
			sb_push(rc->vmHandles, rc->callHandle);
			
			GuiCreateActorRenderCodeHandles(inst->id);
		}
		// error
		else
		{
			rc->type = ACTOR_RENDER_CODE_TYPE_VM_ERROR;
			rc->vmErrorMessage = (result == WREN_RESULT_COMPILE_ERROR)
				? "compile error"
				: "runtime error"
			;
		}
	}
	
	return gRenderCodeDrewSomething;
}

#define PUT_32_BE(BYTES, OFFSET, DATA) { \
	((uint8_t*)(BYTES))[OFFSET + 0] = ((DATA) >> 24) & 0xff; \
	((uint8_t*)(BYTES))[OFFSET + 1] = ((DATA) >> 16) & 0xff; \
	((uint8_t*)(BYTES))[OFFSET + 2] = ((DATA) >>  8) & 0xff; \
	((uint8_t*)(BYTES))[OFFSET + 3] = ((DATA) >>  0) & 0xff; \
}
static void DrawInstanceList(sb_array(struct Instance, *instanceList))
{
	float model[16];
	struct Instance *instances;
	GbiGfx setId[] = { gsXPSetId(0), gsSPEndDisplayList() };
	
	if (!instanceList)
		return;
	
	if (!(instances = *instanceList))
		return;
	
	sb_foreach(instances, {
		identity(model);
		{
			mtx_translate_rot(
				(void*)model
				, &(N64Vector3){ UNFOLD_VEC3(each->pos) }
				, &(N64Vector3){
					BinToRad(each->xrot)
					, BinToRad(each->yrot) - DegToRad(90) // correct model rotation
					, BinToRad(each->zrot)
				}
			);
			
			// scale non-positioning components
			for (int i = 0; i < 12; ++i)
				model[i] *= 0.025f;
		}
		
		uint32_t renderGroup = RENDERGROUP_INST | eachIndex;
		worldRayData.currentInstance = each;
		if (GizmoHasFocus(gState.gizmo) && gGui->selectedInstance == each) // don't raycast onto self
			renderGroup = RENDERGROUP_IGNORE;
		PUT_32_BE(setId, 4, renderGroup);
		
		n64_draw_dlist(setId);
		
		// reset certain instance vars
		// TODO might want to also check rot/params, would avoid
		//      needing ClearFaceSnapVector() in some instances
		if (each->id != each->prev.id)
		{
			each->faceSnapVector = ((Vec3f){0, 0, 0});
		}
		
		// rendercode
		if (!RenderCodeGo(each))
		{
			// draw this actor
			DrawDefaultActorPreview(each);
			
			// inline, less accurate rotations
			//n64_mtx_model(model);
			//n64_segment_set(0x06, meshPrismArrow);
			//n64_draw_dlist(&meshPrismArrow[0x100]);
		}
		
		// prev = current
		each->prev.id = each->id;
	});
}

void WindowMainLoop(struct Scene *scene)
{
	gState.input = &gInput;
	struct Gizmo *gizmo = GizmoNew();
	gState.gizmo = gizmo;
	GLFWwindow* window;
	bool shouldIgnoreInput = false;
	gSceneP = &scene;
	struct GuiInterop gui = {
		.env = {
			.isFogEnabled = true,
			.isLightingEnabled = true,
			.envPreviewMode = GUI_ENV_PREVIEW_EACH
		}
	};
	gGui = &gui;
	
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	
	// glfw window creation
	// --------------------
	window = glfwCreateWindow(WINDOW_INITIAL_WIDTH, WINDOW_INITIAL_HEIGHT, "z64viewer", NULL, NULL);
	if (window == NULL)
	{
		Die("Failed to create GLFW window");
		glfwTerminate();
		return;
	}
	glfwMakeContextCurrent(window);
	
	// callbacks
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetDropCallback(window, drop_callback);
	glfwSetCharCallback(window, char_callback);
	glfwSetKeyCallback(window, key_callback);
	
	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		Die("Failed to initialize GLAD");
		return;
	}
	
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	GuiInit(window);
	Matrix_Init();
	
	struct File *test = 0;
	if (false) {
		const char *testFn = "bin/test.bin";
		testFn = "bin/test1.bin";
		if (FileExists(testFn))
			test = FileFromFilename(testFn);
	}
	
	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		//LogDebug("-- loop ----");
		
		{
			sGameplayFrames += gInput.delta_time_sec * (20.0);
			
			TexAnimSetGameplayFrames(sGameplayFrames);
		}
		
		//ZeldaLight *light = &scene->headers[0].refLights[1]; // 1 = daytime
		EnvLightSettings settings = GetEnvironment(scene, &gui);
		ZeldaLight *result = (void*)&settings;
		
		float bgcolor[3] = { UNFOLD_RGB_EXT(result->fog, / 255.0f) };
		// input
		// -----
		processInput(window);
		
		// render
		// ------
		glDepthMask(GL_TRUE);
		glClearColor(bgcolor[0], bgcolor[1], bgcolor[2], 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// skipping this way is fine for now
		if (!scene)
			goto L_skipSceneRender;
		
		// mvp matrix
		float model[16];
		
		identity(model);
		
		// ignore 3d camera controls if gui has focus
		gState.hasCameraMoved = false;
		if (shouldIgnoreInput == false && scene)
		{
			//bool wasControllingCamera = gInput.mouse.isControllingCamera;
			Matrix viewMtxOld = gState.viewMtx;
			
			camera_flythrough(&gState.viewMtx);
			
			// assume camera is being turned using the mouse
			if (gInput.mouseOld.button.left
				&& memcmp(&gState.viewMtx, &viewMtxOld, sizeof(viewMtxOld))
			)
			{
				gInput.mouse.isControllingCamera = true;
				gState.hasCameraMoved = true;
			}
			
			// old instance selection
		#if 0
			// consume left-click
			if (gInput.mouse.clicked.left)
			{
				gInput.mouse.clicked.left = false;
				gInput.mouse.isControllingCamera = false;
				
				// click an instance to select it
				if (!wasControllingCamera && !GizmoHasFocus(gizmo))
				{
					RayLine ray = WindowGetCursorRayLine();
					GizmoRemoveChildren(gizmo);
					gGui->selectedInstance = 0;
					
					RaycastInstanceList(gGui->actorList, gizmo, ray);
					RaycastInstanceList(gGui->spawnList, gizmo, ray);
					RaycastInstanceList(gGui->doorList, gizmo, ray);
				}
				// ref:
				/*
				for (var_t i = 0; i < room->actorList.num; i++) {
					Actor* actor = Arli_At(&room->actorList, i);
					
					veccpy(&actor->sph.pos, &actor->pos);
					actor->sph.pos.y += 10.0f;
					actor->sph.r = 20;
					
					if (Col3D_LineVsSphere(&ray, &actor->sph, &p)) {
						if (actor->state & ACTOR_SELECTED) {
							selHit = actor;
							
							continue;
						}
						
						selectedActor = actor;
					}
				}
				*/
			}
		#endif
		}
		
		// fog toggle
		if (!gui.env.isFogEnabled)
		{
			result->fog_far = 12800;
			result->fog_near = 1000;
		}
		
		// lighting toggles
		if (!gui.env.isLightingEnabled)
		{
			result->ambient = (ZeldaRGB) { -1, -1, -1 };
		}
		
		projection(&gState.projMtx, gState.winWidth, gState.winHeight, PROJ_NEAR, result->fog_far/*12800*/, gState.cameraFly.fovy);
		
		Matrix_MtxFMtxFMult(&gState.projMtx, &gState.viewMtx, &gState.projViewMtx);
		
		n64_update_tick();
		n64_buffer_init();
		
		n64_culling(false);
		n64_mtx_model(model);
		n64_mtx_view(&gState.viewMtx);
		n64_mtx_projection(&gState.projMtx);
		
		//goto L_onlyGizmo;
		
		ZeldaMatrix zmtx;
		
		// new
		n64_fog(result->fog_near, 1000, UNFOLD_RGB(result->fog));
		n64_light_bind_dir(UNFOLD_VEC3(result->diffuse_a_dir), UNFOLD_RGB(result->diffuse_a));
		n64_light_bind_dir(UNFOLD_VEC3(result->diffuse_b_dir), UNFOLD_RGB(result->diffuse_b));
		n64_light_set_ambient(UNFOLD_RGB(result->ambient));
		
		// old
		//n64_fog(light->fog_near, 1000, UNFOLD_RGB(light->fog));
		//DoLights(light);
		//n64_light_set_ambient(255, 255, 255); // for easy testing
		/*
		n64_light_bind_dir(UNFOLD_VEC3(light->diffuse_a_dir), UNFOLD_RGB(light->diffuse_a));
		n64_light_bind_dir(UNFOLD_VEC3(light->diffuse_b_dir), UNFOLD_RGB(light->diffuse_b));
		n64_light_set_ambient(UNFOLD_RGB(light->ambient));
		*/
		
		mat44_to_matn64((void*)&zmtx, (void*)model);
		
		// generate billboard matrices
		{
			Matrix inverse_mv;
			static uint8_t billboards[0x80]; // static storage so it doesn't expire (is used beyond this scope)
			uint8_t *sphere = billboards;
			uint8_t *cylinder = billboards + 0x40;
			
			// sphere = inverse of view mtx w/ some parts reverted to identity
			memcpy(&inverse_mv, &gState.viewMtx, sizeof(gState.viewMtx));
			inverse_mv.mf[0][3] = inverse_mv.mf[1][3] = inverse_mv.mf[2][3]
				= inverse_mv.mf[3][0] = inverse_mv.mf[3][1] = inverse_mv.mf[3][2]
				= 0.0f;
			Matrix_Transpose(&inverse_mv);
			
			// store a copy in Matrix format for later
			gBillboardMatrix[0] = inverse_mv;
			Matrix cyl = inverse_mv;
			cyl.xy = 0; cyl.yy = 1; cyl.zy = 0;
			gBillboardMatrix[1] = cyl;
			
			// convert to n64 matrix format
			mat44_to_matn64(billboards, (void*)&inverse_mv);
			
			// cylinder billboard = sphere w/ up vector reverted to identity
			{
				memcpy(cylinder, sphere, 0x40);
				
				// integer parts
				WBE16(&cylinder[0x08], 0); // x
				WBE16(&cylinder[0x0a], 1); // y
				WBE16(&cylinder[0x0c], 0); // z
				
				// fractional parts
				WBE16(&cylinder[0x28], 0); // x
				WBE16(&cylinder[0x2a], 0); // y
				WBE16(&cylinder[0x2c], 0); // z
			}
			
			// billboard matrices live in segment 0x01
			gSPSegment(POLY_OPA_DISP++, 0x1, billboards);
			gSPSegment(POLY_XLU_DISP++, 0x1, billboards);
			gRenderCodeBillboards = billboards;
		}
		
		sb_foreach(scene->rooms, {
			void *sceneSegment = scene->file->data;
			void *roomSegment = each->file->data;
			struct SceneHeader *sceneHeader = &scene->headers[0];
			
			gXPSetId(POLY_OPA_DISP++, RENDERGROUP_ROOM | eachIndex);
			gXPSetId(POLY_XLU_DISP++, RENDERGROUP_ROOM | eachIndex);
			
			gSPDisplayList(POLY_OPA_DISP++, n64_material_setup_dl[0x19]);
			gSPSegment(POLY_OPA_DISP++, 2, sceneSegment);
			gSPSegment(POLY_OPA_DISP++, 3, roomSegment);
			gDPSetEnvColor(POLY_OPA_DISP++, 0x80, 0x80, 0x80, 0x80);
			gSPMatrix(POLY_OPA_DISP++, &zmtx, G_MTX_MODELVIEW | G_MTX_LOAD);
			
			gSPDisplayList(POLY_XLU_DISP++, n64_material_setup_dl[0x19]);
			gSPSegment(POLY_XLU_DISP++, 2, sceneSegment);
			gSPSegment(POLY_XLU_DISP++, 3, roomSegment);
			gDPSetEnvColor(POLY_XLU_DISP++, 0x80, 0x80, 0x80, 0x80);
			gSPMatrix(POLY_XLU_DISP++, &zmtx, G_MTX_MODELVIEW | G_MTX_LOAD);
			
			// animate water in forest test scene
			if (scene->file->size == 0x11240)
				AnimateTheWater();
			else if (sceneHeader->mm.sceneSetupType != -1)
				TexAnimSetupSceneMM(sceneHeader->mm.sceneSetupType, sceneHeader->mm.sceneSetupData);
			
			if (each->headers[0].meshFormat == 2)
			{
				DrawRoomCullable(&each->headers[0], result->fog_far, ROOM_DRAW_OPA | ROOM_DRAW_XLU);
				//break; // process only room[0] for now
				continue;
			}
			
			typeof(each->headers[0].displayLists) dls = each->headers[0].displayLists;
			sb_foreach(dls, {
				if (each->opa)
					gSPDisplayList(POLY_OPA_DISP++, each->opa);
				if (each->xlu)
					gSPDisplayList(POLY_XLU_DISP++, each->xlu);
			});
			
			//if (each == &scene->rooms[0])
			//	gSPDisplayList(POLY_OPA_DISP++, 0x03004860);
		});
		
		// draw arrow
		/*
		ZeldaMatrix zmtx2;
		identity(model);
		for (int i = 0; i < 15; ++i) model[i] *= 0.1f;
		mtx_to_zmtx((Matrix*)model, &zmtx2);
		gSPMatrix(POLY_OPA_DISP++, &zmtx2, G_MTX_MODELVIEW | G_MTX_LOAD);
		
		gSPSegment(POLY_OPA_DISP++, 0x06, meshPrismArrow);
		gSPDisplayList(POLY_OPA_DISP++, matBlank);
		gSPDisplayList(POLY_OPA_DISP++, 0x06000100);
		*/
		
		if (test)
		{
			gSPSegment(POLY_OPA_DISP++, 0x06, test->data);
			
			if (test->size == 0x37A00 || test->size == 0x37800)
				gSPDisplayList(POLY_OPA_DISP++, 0x06021F78);
			else if (test->size == 0x1E250)
				gSPDisplayList(POLY_OPA_DISP++, 0x06016480);
		}
		
		if (gSkelAnimeTest.skeleton)
		{
			float scale = 0.02;
			Matrix_Translate(-100, 0, 0, MTXMODE_NEW);
			//Matrix_RotateY_s(rot.y, MTXMODE_APPLY);
			//Matrix_RotateX_s(rot.x, MTXMODE_APPLY);
			//Matrix_RotateZ_s(rot.z, MTXMODE_APPLY);
			Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
			gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtxN64(), G_MTX_MODELVIEW | G_MTX_LOAD);
			SkelAnime_Update(&gSkelAnimeTest, gInput.delta_time_sec * (20.0));
			SkelAnime_Draw(&gSkelAnimeTest, SKELANIME_TYPE_FLEX);
		}
		
		// if mouse has moved or ctrl key state has changed,
		// perform raycast against all visual geometry
		bool isRaycasting = false;
		worldRayData.isSelectingInstance = false;
		if ((GizmoHasFocus(gizmo)
			&& gInput.key.lctrl
			&& (gInput.mouse.vel.x
				|| gInput.mouse.vel.y
				|| gInput.key.lctrl != gInput.keyOld.lctrl
			))
			|| gInput.mouse.clicked.right
			|| gState.deferredInstancePaste
		)
		{
			worldRayData.ray = WindowGetCursorRayLine();
			worldRayData.ray.nearest = FLT_MAX;
			worldRayData.useSnapAngle = false;
			worldRayData.renderGroup = RENDERGROUP_ROOM;
			n64_set_tri_callback(&worldRayData, CameraRayCallback);
			isRaycasting = true;
		}
		
		n64_buffer_flush(true);
		
		// snap rotation
		if (worldRayData.useSnapAngle
			&& gGui->selectedInstance
			&& Vec3f_IsNonZero(gGui->selectedInstance->faceSnapVector)
		)
		{
			Triangle tri = worldRayData.snapAngleTri;
			Vec3f triNormal = Vec3f_NormalFromTriangleVertices(tri.v[0], tri.v[1], tri.v[2]);
			Vec3s triNormal16 = (Vec3s) { UNFOLD_VEC3_EXT(triNormal, * (32768.0f / M_PI)) };
			Vec3f result = Vec3f_FaceNormalToYawPitch64(triNormal);
			//Vec3f up = { 0, 1, 0 };
			//Vec3f up = { 0, 0, 1 }; // eye switch
			struct Instance *inst = gGui->selectedInstance;
			
			// if surface angle has changed
			if (Vec3s_DistXYZ(triNormal16, inst->prev.triNormal16) > 1)
			{
				// provide current instance rotation, to preserve
				// original rotation if it's already acceptable
				result = (Vec3f) { BinToRad(inst->xrot), BinToRad(inst->yrot), BinToRad(inst->zrot) };
				
				inst->prev.triNormal16 = triNormal16;
				result = Vec3f_BruteforceEulerAnglesTowardsDirection(result, triNormal, inst->faceSnapVector);
				//LogDebug("BruteEulerAngles output = %f %f %f", UNFOLD_VEC3(result));
				
				inst->xrot = RadToBin(result.x);
				inst->yrot = RadToBin(result.y);
				inst->zrot = RadToBin(result.z);
				//LogDebug("BruteEulerAngles result = %04x %04x %04x", inst->xrot, inst->yrot, inst->zrot);
			}
			worldRayData.useSnapAngle = false;
		}
		
		// left-click an instance to select it
		// TODO consolidate it into the above block
		if (shouldIgnoreInput == false
			&& gInput.mouse.clicked.left
			&& !GizmoHasFocus(gizmo)
			&& !gInput.mouse.isControllingCamera
		)
		{
			// setup
			if (!isRaycasting)
			{
				worldRayData.ray = WindowGetCursorRayLine();
				worldRayData.ray.nearest = FLT_MAX;
				n64_set_tri_callback(&worldRayData, CameraRayCallback);
				worldRayData.isSelectingInstance = true;
				isRaycasting = true;
			}
			
			GizmoRemoveChildren(gizmo);
			gGui->selectedInstance = 0;
		}

		// released lmb
		if (gInput.mouse.clicked.left)
			gInput.mouse.isControllingCamera = false;

		if (isRaycasting)
			worldRayData.isSelectingInstance = true,
			worldRayData.renderGroup = RENDERGROUP_INST;
		
		// draw shape at each instance position
		n64_draw_dlist(matBlank);
		static GbiGfx gfxGreen[] = { gsDPSetPrimColor(0, 0, 0, 255, 0, 255), gsSPEndDisplayList() };
		static GbiGfx gfxRed[] = { gsDPSetPrimColor(0, 0, 255, 0, 0, 255), gsSPEndDisplayList() };
		n64_draw_dlist(gfxEnableXray);
		DrawInstanceList(gGui->actorList);
		n64_draw_dlist(gfxGreen);
		DrawInstanceList(gGui->spawnList);
		n64_draw_dlist(gfxRed);
		DrawInstanceList(gGui->doorList);
		n64_draw_dlist(gfxDisableXray);
		
		n64_set_tri_callback(0, 0);
		
		// test
		if (false)
		{
			//{ Matrix model; Matrix_Push(); Matrix_Translate(0, 0, 0, MTXMODE_NEW); Matrix_Get(&model); Matrix_Pop(); n64_mtx_model(&model); }
			Matrix model;
			//identity((void*)&model);
			Matrix_Push(); Matrix_Translate(0, 0, 0, MTXMODE_NEW); Matrix_Get(&model); Matrix_Pop();
			n64_mtx_model(&model);
			n64_draw_dlist(matBlank);
			n64_segment_set(0x06, meshPrismArrow);
			n64_draw_dlist(&meshPrismArrow[0x100]);
		}
		
		// gizmo
		GizmoDraw(gizmo, &gState.cameraFly);
		
		// consume right click (after raycasts)
		if (gInput.mouse.clicked.right)
		{
			gGui->rightClickedInViewport = true;
			gGui->rightClickedRenderGroup = worldRayData.renderGroupClicked;
			gInput.mouse.clicked.right = false;
			gInput.mouse.isControllingCamera = false;
			
			// spawn thing at position raycast resolved to
			if (worldRayData.ray.nearest < FLT_MAX)
			{
				gGui->newSpawnPos = worldRayData.pos;
				//LogDebug("newSpawnPos = %f %f %f", UNFOLD_VEC3(gGui->newSpawnPos));
			}
		}
		
		if (gState.deferredInstancePaste)
		{
			// paste at deferred raycast position
			if (worldRayData.ray.nearest < FLT_MAX)
				gGui->newSpawnPos = worldRayData.pos;
			
			gState.deferredInstancePaste = false;
			WindowTryInstancePaste(true, false);
		}
		
	L_skipSceneRender:
		// draw the ui
		GuiDraw(window, scene, &gui);
		shouldIgnoreInput = GuiHasFocus();
		if (!GuiHasFocusKeyboard()) // ignore gizmo keyboard shortcuts if gui has keyboard focus
			GizmoUpdate(gizmo, &worldRayData.pos);
		shouldIgnoreInput |= GizmoHasFocus(gizmo);
		shouldIgnoreInput |= gGui->rightClickedInViewport;
		gState.cameraIgnoreLMB = GizmoIsHovered(gizmo);
		
		// changed selections using ui
		{
			ON_CHANGE(gGui->selectedInstance)
			{
				struct Instance *inst = gGui->selectedInstance;
				
				GizmoRemoveChildren(gizmo);
				if (inst)
				{
					GizmoSetPosition(gizmo, UNFOLD_VEC3(inst->pos));
					GizmoAddChild(gizmo, &inst->pos);
				}
			}
		}
		
		// consume left-click if nothing already has
		gInput.mouse.clicked.left = false;
		gInput.mouse.clicked.right = false;
		gInput.mouse.vel.x = gInput.mouse.vel.y = 0;
		gGui->rightClickedInViewport = false;

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		memcpy(&gInput.keyOld, &gInput.key, sizeof(gInput.key));
		memcpy(&gInput.mouseOld, &gInput.mouse, sizeof(gInput.mouse));
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	
	glfwTerminate();
	
	// cleanup
	if (scene)
		SceneFree(scene);
	if (gizmo)
		free(gizmo);
}

