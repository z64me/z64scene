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

RayLine WindowGetCursorRayLine(void);

#endif /* Z64SCENE_WINDOW_H_INCLUDED */

