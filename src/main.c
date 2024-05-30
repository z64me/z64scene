//
// main.c
//
// z64scene entry point
// it all starts here
//

#include <stdio.h>

#include "misc.h"

#define SCENE_PREFIX "termina_"

extern void WindowMainLoop(struct Scene *scene);

int main(void)
{
	struct Scene *scene = SceneFromFilename("bin/" SCENE_PREFIX "scene.zscene");
	struct Object *object = ObjectFromFilename("bin/object.zobj");
	
	for (int i = 0; i < scene->headers[0].numRooms; ++i)
	{
		char tmp[64];
		
		snprintf(tmp, sizeof(tmp), "bin/" SCENE_PREFIX "room_%d.zroom", i);
		
		SceneAddRoom(scene, RoomFromFilename(tmp));
	}
	
	fprintf(stderr, "%p\n", &scene->test);
	
	WindowMainLoop(scene);
	
	// cleanup
	SceneFree(scene);
	ObjectFree(object);
	
	return 0;
}

