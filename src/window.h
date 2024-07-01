//
// window.h
//
// the main window and some
// things pertaining to it
//

#ifndef Z64SCENE_WINDOW_H_INCLUDED
#define Z64SCENE_WINDOW_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>
#include "extmath.h"

struct CameraFly
{
	Vec3f eye;
	Vec3f lookAt;
	float fovy;
};

struct Input
{
	struct
	{
		struct
		{
			int x;
			int y;
		} pos, vel;
		struct
		{
			int dy; /* delta y */
		} wheel;
		struct
		{
			int left;
			int right;
		} button;
		struct
		{
			bool left;
			bool right;
		} clicked;
		bool isControllingCamera;
	} mouse, mouseOld;
	struct
	{
		int w;
		int a;
		int s;
		int d;
		int q;
		int e;
		int g;
		int lshift;
		int lctrl;
		int enter;
		int escape;
	} key, keyOld;
	struct
	{
		char text[512];
		unsigned int maxChars;
		bool isEnabled;
	} textInput;
	float delta_time_sec;
};

extern struct Input gInput;

RayLine WindowGetRayLine(Vec2f point);
RayLine WindowGetCursorRayLine(void);
void WindowClipPointIntoView(Vec3f* a, Vec3f normal);
Vec2f WindowGetLocalScreenPos(Vec3f point);
bool WindowTryInstanceDuplicate(void);

struct Object *WindowLoadObject(const char *fn);

#endif /* Z64SCENE_WINDOW_H_INCLUDED */

