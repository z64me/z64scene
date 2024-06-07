#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>

#include "misc.h"
#include "extmath.h"
#include "gizmo.h"
#include "gui.h"
#include "window.h"
#include "texanim.h"
#include <n64.h>
#include <n64types.h>

#define NOC_FILE_DIALOG_IMPLEMENTATION
#include "noc_file_dialog.h"

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
	Matrix viewProjMtx;
	int winWidth;
	int winHeight;
	struct CameraFly cameraFly;
	bool cameraIgnoreLMB;
} gState = {
	.winWidth = WINDOW_INITIAL_WIDTH
	, .winHeight = WINDOW_INITIAL_HEIGHT
	, .cameraFly = {
		.fovy = WINDOW_INITIAL_FOVY
	}
};

struct CameraRay
{
	RayLine ray;
	Vec3f pos;
	Vec3f dir;
};

void CameraRayCallback(void *udata, const N64Tri *tri64)
{
	struct CameraRay *ud = udata;
	Triangle tri = {
		.v = {
			{ UNFOLD_VEC3(tri64->vtx[0]->pos) }
			,{ UNFOLD_VEC3(tri64->vtx[1]->pos) }
			,{ UNFOLD_VEC3(tri64->vtx[2]->pos) }
		}
		, .cullBackface = tri64->cullBackface
		, .cullFrontface = tri64->cullFrontface
	};
	
	Col3D_LineVsTriangle(
		&ud->ray
		, &tri
		, &ud->pos
		, &ud->dir
		, tri64->cullBackface
		, tri64->cullFrontface
	);
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
	
	switch (key)
	{
		case GLFW_KEY_W:
			gInput.key.w = pressed;
			break;
		
		case GLFW_KEY_A:
			gInput.key.a = pressed;
			break;
		
		case GLFW_KEY_S:
			gInput.key.s = pressed;
			break;
		
		case GLFW_KEY_D:
			gInput.key.d = pressed;
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
		
		case GLFW_KEY_O:
			// ctrl + o ; Ctrl+O
			if (pressed && (mods & GLFW_MOD_CONTROL))
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
	
	//fprintf(stderr, "filename = %s, extension = %s\n", filename, extension);
	
	char *extensionLower = Strdup(extension + 1);
	for (char *c = extensionLower; *c; ++c)
		*c = tolower(*c);
	
	if (!strcmp(extensionLower, "zscene"))
		WindowLoadScene(filename);
	
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
	
	//fprintf(stderr, "%f %f %f\n", UNFOLD_ARRAY_3(float, look));
	memcpy(&gState.cameraFly.eye, pos, sizeof(pos));
	//memcpy(&gState.cameraFly.lookAt, look, sizeof(look));
	gState.cameraFly.lookAt = (Vec3f) {
		pos[0] + look[0] * PROJ_NEAR,
		pos[1] + look[1] * PROJ_NEAR,
		pos[2] + look[2] * PROJ_NEAR
	};
	
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

struct Scene *WindowLoadScene(const char *fn)
{
	fprintf(stderr, "%s\n", fn);
	struct Scene *scene = SceneFromFilenamePredictRooms(fn);
	
	// cleanup
	if (*gSceneP)
		SceneFree(*gSceneP);
	n64_clear_cache();
	
	// view new scene
	*gSceneP = scene;
	
	return scene;
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

	// Matrix_MtxToMtxF(&this->view.projection, &this->viewProjectionMtxF);
	Matrix viewProjectionMtxF = gState.projViewMtx;//viewProjMtx;
	Vec3f projectionMtxFDiagonal = {
		viewProjectionMtxF.xx, viewProjectionMtxF.yy, viewProjectionMtxF.zz
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
		
		//projectedPos.z *= 1.0f / projectionMtxFDiagonal.z; // original math
		// commented it out, using camera position hack for now
		
		//Vec4f test = WindowGetLocalScreenVec(pos);
		//fprintf(stderr, "test %f %f\n", test.z, test.w);
		//fprintf(stderr, " -> %f\n", projectedPos.z);
		
		float var_fv1 = ABS_ALT(cullable->radius);
		
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
	
	//fprintf(stderr, "draw %d / %d\n", (int)(insert - linkedEntriesBuffer), numEntries);

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

void WindowMainLoop(struct Scene *scene)
{
	gState.input = &gInput;
	struct Gizmo *gizmo = GizmoNew();
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
	struct CameraRay worldRayData = { 0 };
	
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
	if (true) {
		const char *testFn = "bin/test.bin";
		testFn = "bin/test1.bin";
		if (FileExists(testFn))
			test = FileFromFilename(testFn);
	}
	
	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		//fprintf(stderr, "-- loop ----\n");
		
		{
			static double sGameplayFrames = 0;
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
		glClearColor(bgcolor[0], bgcolor[1], bgcolor[2], 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// skipping this way is fine for now
		if (!scene)
			goto L_skipSceneRender;
		
		// mvp matrix
		float model[16];
		
		identity(model);
		
		// ignore 3d camera controls if gui has focus
		if (shouldIgnoreInput == false && scene)
		{
			bool wasControllingCamera = gInput.mouse.isControllingCamera;
			Matrix viewMtxOld = gState.viewMtx;
			
			camera_flythrough(&gState.viewMtx);
			
			// assume camera is being turned using the mouse
			if (gInput.mouseOld.button.left
				&& memcmp(&gState.viewMtx, &viewMtxOld, sizeof(viewMtxOld))
			)
				gInput.mouse.isControllingCamera = true;
			
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
					
					sb_foreach(scene->rooms, {
						struct RoomHeader *header = &each->headers[0];
						sb_foreach(header->instances, {
							if (Col3D_LineVsSphere(
								&ray
								, &(Sphere){
									.pos = (Vec3f){ each->x, each->y, each->z }
									, .r = 20
								}
								, 0
							)) {
								// TODO construct a list of collisions and choose the nearest instance
								// TODO if multiple instances overlap, each click should cycle through them
								// TODO update gui instances tab
								GizmoSetPosition(gizmo, each->x, each->y, each->z);
								GizmoAddChild(gizmo, &each->pos);
								GuiCallbackActorGrabbed(each->id);
							}
						});
					});
				}
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
		
		mtx_to_zmtx((Matrix*)model, &zmtx);
		
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
			
			// FIXME viewProjMtx test for culling
			Matrix_Push(); {
				//Matrix_MtxToMtxF(&this->view.projection, &this->viewProjectionMtxF);
				Matrix_Mult(&gState.projMtx, MTXMODE_NEW);
				// The billboard is still a viewing matrix at this stage
				Matrix billboardF;
				Matrix_MtxToMtxF((void*)billboards, &billboardF);
				Matrix_Mult(&billboardF, MTXMODE_APPLY);
				Matrix_Get(&gState.viewProjMtx);
			} Matrix_Pop();
		}
		
		sb_foreach(scene->rooms, {
			void *sceneSegment = scene->file->data;
			void *roomSegment = each->file->data;
			struct SceneHeader *sceneHeader = &scene->headers[0];
			
			gSPDisplayList(POLY_OPA_DISP++, n64_material_setup_dl[0x19]);
			gSPSegment(POLY_OPA_DISP++, 2, sceneSegment);
			gSPSegment(POLY_OPA_DISP++, 3, roomSegment);
			gDPSetEnvColor(POLY_OPA_DISP++, 0x80, 0x80, 0x80, 0x80);
			//gSPMatrix(POLY_OPA_DISP++, &zmtx, G_MTX_MODELVIEW | G_MTX_LOAD);
			
			gSPDisplayList(POLY_XLU_DISP++, n64_material_setup_dl[0x19]);
			gSPSegment(POLY_XLU_DISP++, 2, sceneSegment);
			gSPSegment(POLY_XLU_DISP++, 3, roomSegment);
			gDPSetEnvColor(POLY_XLU_DISP++, 0x80, 0x80, 0x80, 0x80);
			//gSPMatrix(POLY_XLU_DISP++, &zmtx, G_MTX_MODELVIEW | G_MTX_LOAD);
			
			// animate water in forest test scene
			if (scene->file->size == 0x11240)
				AnimateTheWater();
			else if (sceneHeader->mm.sceneSetupType != -1)
				TexAnimSetupSceneMM(sceneHeader->mm.sceneSetupType, sceneHeader->mm.sceneSetupData);
			
			DrawRoomCullable(&each->headers[0], result->fog_far, ROOM_DRAW_OPA | ROOM_DRAW_XLU);
			continue;
			
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
		
		// if mouse has moved or ctrl key state has changed,
		// perform raycast against all visual geometry
		if (GizmoHasFocus(gizmo)
			&& gInput.key.lctrl
			&& (gInput.mouse.vel.x
				|| gInput.mouse.vel.y
				|| gInput.key.lctrl != gInput.keyOld.lctrl
			)
		)
		{
			worldRayData.ray = WindowGetCursorRayLine();
			worldRayData.ray.nearest = FLT_MAX;
			n64_set_tri_callback(&worldRayData, CameraRayCallback);
		}
		
		n64_buffer_flush(true);
		
		n64_set_tri_callback(0, 0);
		
		// draw shape at each instance position
		n64_draw_dlist(matBlank);
		static GbiGfx gfxEnableXray[] = { gsXPMode(0, GX_MODE_OUTLINE), gsSPEndDisplayList() };
		static GbiGfx gfxDisableXray[] = { gsXPMode(GX_MODE_OUTLINE, 0), gsSPEndDisplayList() };
		n64_draw_dlist(gfxEnableXray);
		sb_foreach(scene->rooms, {
			struct RoomHeader *header = &each->headers[0];
			sb_foreach(header->instances, {
				each->x = rintf(each->pos.x);
				each->y = rintf(each->pos.y);
				each->z = rintf(each->pos.z);
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
				
				n64_mtx_model(model);
				n64_segment_set(0x06, meshPrismArrow);
				n64_draw_dlist(&meshPrismArrow[0x100]);
			});
		});
		n64_draw_dlist(gfxDisableXray);
		
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
		
	L_skipSceneRender:
		// draw the ui
		GuiDraw(window, scene, &gui);
		shouldIgnoreInput = GuiHasFocus();
		if (!GuiHasFocusKeyboard()) // ignore gizmo keyboard shortcuts if gui has keyboard focus
			GizmoUpdate(gizmo, &worldRayData.pos);
		shouldIgnoreInput |= GizmoHasFocus(gizmo);
		gState.cameraIgnoreLMB = GizmoIsHovered(gizmo);
		
		// consume left-click if nothing already has
		gInput.mouse.clicked.left = false;
		gInput.mouse.clicked.right = false;
		gInput.mouse.vel.x = gInput.mouse.vel.y = 0;

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

