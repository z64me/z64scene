//
// main.c
//
// z64scene entry point
// it all starts here
//

#include <stdio.h>

#include "misc.h"

int main(void)
{
	struct Scene *scene = SceneFromFilename("bin/scene.zscene");
	struct Object *object = ObjectFromFilename("bin/object.zobj");
	
	for (int i = 0; i < scene->headers[0].numRooms; ++i)
	{
		char tmp[64];
		
		snprintf(tmp, sizeof(tmp), "bin/room_%d.zroom", i);
		
		SceneAddRoom(scene, RoomFromFilename(tmp));
	}
	
	// cleanup
	SceneFree(scene);
	ObjectFree(object);
	
	return 0;
}

