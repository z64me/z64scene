#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "misc.h"
#include <n64.h>

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
		static float bgcolor[3] = { 0.5, 0.5, 0.5 };
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
		//n64_fog(light->fog_near, 1000, unfold_rgb(light->fog));
		n64_mtx_model(model);
		n64_mtx_view(view);
		n64_mtx_projection(p);
		
		ZeldaMatrix zmtx;
		
		n64_light_set_ambient(255, 255, 255);
		/*
		n64_light_bind_dir(unfold_vec3(light->diffuse_a_dir), unfold_rgb(light->diffuse_a));
		n64_light_bind_dir(unfold_vec3(light->diffuse_b_dir), unfold_rgb(light->diffuse_b));
		n64_light_set_ambient(unfold_rgb(light->ambient));
		*/
		
		mtx_to_zmtx((Matrix*)model, &zmtx);
		
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

