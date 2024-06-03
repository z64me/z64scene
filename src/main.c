//
// main.c
//
// z64scene entry point
// it all starts here
//

#include <stdio.h>

#include "misc.h"

extern void WindowMainLoop(struct Scene *scene);

int main(int argc, char *argv[])
{
	struct Scene *scene = 0;
	
	ExePath(argv[0]);
	
	if (argc == 2)
		scene = SceneFromFilenamePredictRooms(argv[1]);
	
	WindowMainLoop(scene);
	
	return 0;
}

