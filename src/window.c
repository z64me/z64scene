#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "misc.h"
#include <n64.h>

// write bigendian bytes
#define WBE16(DST, V) { ((uint8_t*)DST)[0] = (V) >> 8; ((uint8_t*)DST)[1] = (V) & 0xff; }

#define FLYTHROUGH_CAMERA_IMPLEMENTATION
#include "flythrough_camera.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

// settings
unsigned int gWindowWidth = 800;
unsigned int gWindowHeight = 600;

struct
{
	struct
	{
		struct
		{
			int x;
			int y;
		} pos;
		struct
		{
			int dy; /* delta y */
		} wheel;
		struct
		{
			int left;
			int right;
		} button;
	} mouse;
	struct
	{
		int w;
		int a;
		int s;
		int d;
		int lshift;
	} key;
	float delta_time_sec;
} gInput = {0};

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	
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
	gWindowWidth = width;
	gWindowHeight = height;
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
	}
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
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
			break;
		
		case GLFW_MOUSE_BUTTON_LEFT:
			gInput.mouse.button.left = pressed;
			break;
	}
}

static inline void *camera_flythrough(float m[16])
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
	
	int activated = gInput.mouse.button.left;
	float speed = 1000;
	
	flythrough_camera_update(
		pos, look, up, m
		, gInput.delta_time_sec
		, speed * (gInput.key.lshift ? 2.0f : 1.0f)
		, 0.5f * activated
		, 80.0f
		, cursor.x - oldcursor.x, cursor.y - oldcursor.y
		, gInput.key.w
		, gInput.key.a
		, gInput.key.s
		, gInput.key.d
		, 0//gInput.key.space
		, 0//gInput.key.lctrl
		, 0
	);
	
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

static float *projection(float dst[16], float winw, float winh, float near, float far)
{
	float *m = dst;
	float aspect;
	float f;
	float iNF;

	/* intialize projection matrix */
	aspect = winw / winh;
	f      = 1.0 / tan(60.0f * (3.14159265359f / 180.0f) * 0.5f);
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

typedef struct Matrix {
	float xx, yx, zx, wx;
	float xy, yy, zy, wy;
	float xz, yz, zz, wz;
	float xw, yw, zw, ww;
} Matrix;

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

static inline void mat44_to_matn64(unsigned char *dest, float src[16])
{
#define    FTOFIX32(x)    (long)((x) * (float)0x00010000)
	int32_t t;
	unsigned char *intpart = dest;
	unsigned char *fracpart = dest + 0x20;
	for (int i=0; i < 4; ++i)
	{
		for (int k=0; k < 4; ++k)
		{
			t = FTOFIX32(src[4*i+k]);
			short ip = (t >> 16) & 0xFFFF;
			intpart[0]  = (ip >> 8) & 255;
			intpart[1]  = (ip >> 0) & 255;
			fracpart[0] = (t >>  8) & 255;
			fracpart[1] = (t >>  0) & 255;
			intpart  += 2;
			fracpart += 2;
		}
	}
}

typedef float MtxF_t[4][4];
typedef union {
	MtxF_t mf;
	struct {
		float xx, yx, zx, wx,
		    xy, yy, zy, wy,
		    xz, yz, zz, wz,
		    xw, yw, zw, ww;
	};
} MtxF;

void Matrix_Transpose(MtxF* mf) {
	float temp;
	
	temp = mf->yx;
	mf->yx = mf->xy;
	mf->xy = temp;
	
	temp = mf->zx;
	mf->zx = mf->xz;
	mf->xz = temp;
	
	temp = mf->zy;
	mf->zy = mf->yz;
	mf->yz = temp;
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
static EnvLightSettings GetEnvironment(EnvLightSettings *lights)
{
	// testing
	// 0x8001 is noon
	static uint16_t skyboxTime = 0x8001; // gSaveContext.skyboxTime
	static uint16_t dayTime = 0x8001; // gSaveContext.dayTime
	const int speed = 100;
	skyboxTime += speed;
	dayTime += speed;
	
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
		dst.light1Dir[0] = -(Math_SinS(dayTime - CLOCK_TIME(12, 0)) * 120.0f);
		dst.light1Dir[1] = Math_CosS(dayTime - CLOCK_TIME(12, 0)) * 120.0f;
		dst.light1Dir[2] = Math_CosS(dayTime - CLOCK_TIME(12, 0)) * 20.0f;
		
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
			(Math_SinS(dayTime) - 0x8000) * 120.0f,
			(Math_CosS(dayTime) - 0x8000) * 120.0f,
			(Math_CosS(dayTime) - 0x8000) * 20.0f
		};
		
		Vec3_Substract(dirB, (ZeldaVecS8) { 0 }, dirA);
	}
	
	n64_light_bind_dir(UNFOLD_VEC3(dirA), UNFOLD_RGB(light->diffuse_a));
	n64_light_bind_dir(UNFOLD_VEC3(dirB), UNFOLD_RGB(light->diffuse_b));
	n64_light_set_ambient(UNFOLD_RGB(light->ambient));
}

void WindowMainLoop(struct Scene *scene)
{
	GLFWwindow* window;
	
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
	window = glfwCreateWindow(gWindowWidth, gWindowHeight, "z64viewer", NULL, NULL);
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
	
	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		//ZeldaLight *light = &scene->headers[0].refLights[1]; // 1 = daytime
		EnvLightSettings settings = GetEnvironment((void*)scene->headers[0].refLights);
		ZeldaLight *result = (void*)&settings;
		
		float bgcolor[3] = { UNFOLD_RGB_EXT(result->fog, / 255.0f) };
		// input
		// -----
		processInput(window);
		
		// render
		// ------
		glClearColor(bgcolor[0], bgcolor[1], bgcolor[2], 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// mvp matrix
		float model[16];
		float view[16];
		float p[16];
		
		identity(model);
		camera_flythrough(view);
		projection(p, gWindowWidth, gWindowHeight, 10, 12800);
		
		n64_update_tick();
		n64_buffer_init();
		
		n64_culling(false);
		n64_mtx_model(model);
		n64_mtx_view(view);
		n64_mtx_projection(p);
		
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
			MtxF inverse_mv;
			uint8_t billboards[0x80];
			uint8_t *sphere = billboards;
			uint8_t *cylinder = billboards + 0x40;
			
			// sphere = inverse of view mtx w/ some parts reverted to identity
			memcpy(&inverse_mv, view, sizeof(view));
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
		}
		
		sb_foreach(scene->rooms, {
			void *sceneSegment = scene->file->data;
			void *roomSegment = each->file->data;
			
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
		
		n64_buffer_flush(true);

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	
	glfwTerminate();
}

