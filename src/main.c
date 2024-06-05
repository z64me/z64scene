//
// main.c
//
// z64scene entry point
// it all starts here
//

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "misc.h"

extern void WindowMainLoop(struct Scene *scene);

int main(int argc, char *argv[])
{
	struct Scene *scene = 0;
	
	ExePath(argv[0]);
	
	if (argc == 2)
	{
		scene = SceneFromFilenamePredictRooms(argv[1]);
	}
	else if (argc > 2)
	{
		struct File *file = FileFromFilename(argv[1]);
		struct DataBlob *blobs;
		uint32_t skeleton = 0;
		uint32_t animation = 0;
		
		for (int i = 2; i < argc; ++i)
		{
			const char *arg = argv[i];
			const char *next = argv[i + 1];
			
			while (*arg && !isalnum(*arg))
				++arg;
			
			if (!strcmp(arg, "skeleton"))
			{
				if (sscanf(next, "%x", &skeleton) != 1)
					Die("skeleton invalid argument no pointer '%s'", next);
				++i;
			}
			else if (!strcmp(arg, "animation"))
			{
				if (sscanf(next, "%x", &animation) != 1)
					Die("animation invalid argument no pointer '%s'", next);
				++i;
			}
		}
		
		if (!(skeleton && animation))
			Die("please provide a skeleton and an animation");
		
		// get texture blobs
		// very WIP, just want to view texture blobs for now
		blobs = MiscSkeletonDataBlobs(file, 0, skeleton);
		sb_array(struct TextureBlob, texBlobs) = 0;
		TextureBlobSbArrayFromDataBlobs(file, blobs, &texBlobs);
		scene = SceneFromFilenamePredictRooms("bin/scene.zscene");
		scene->textureBlobs = texBlobs;
	}
	
	WindowMainLoop(scene);
	
	return 0;
}

