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
#include "test.h"
#include "project.h"

extern void WindowMainLoop(struct Scene *scene);

int main(int argc, char *argv[])
{
	struct Scene *scene = 0;
	
	// test wren
	TestWren();
	
	// project loader test
	if (true)
	{
		struct Project *project = ProjectNewFromFilename(argv[1]);
		
		FileListPrintAll(project->foldersObject);
		
		ProjectFree(project);
		
		return 0;
	}
	
	// project directory -> file/folder list test
	if (false)
	{
		sb_array(char *, folders) = FileListFromDirectory(argv[1], 0, false, true, true);
		
		if (folders)
		{
			sb_array(char *, objects) = FileListFilterByWithVanilla(folders, "object", ".vanilla");
			
			FileListPrintAll(objects);
			
			if (sb_count(objects) > 0)
			{
				sb_array(char *, files) = FileListFromDirectory(objects[1], 1, true, false, false);
				FileListPrintAll(files);
				FileListFree(files);
			}
			
			FileListFree(objects);
			FileListFree(folders);
		}
		
		return 0;
	}

	// test: open and re-export a single scene (argv[1] -> argv[2])
#if 0
	if (argc >= 2)
	{
		scene = SceneFromFilenamePredictRooms(argv[1]);
		if (argc == 3)
			SceneToFilename(scene, argv[2]);
		SceneFree(scene);
		if (argc == 3)
			fprintf(stderr, "successfully wrote test scene\n");
		else
			fprintf(stderr, "successfully processed input scene\n");
	}
	SceneWriterCleanup();
	return 0;
#endif

	// test: open and re-export a list of scenes
	// one filename per line, e.g.
	// bin/exhaustive/oot/scenes/0 - Dungeon/scene.zscene
	// bin/exhaustive/oot/scenes/1 - House/scene.zscene
	// bin/exhaustive/oot/scenes/2 - Town/scene.zscene
#if 0
	{
		//const char *fn = "bin/exhaustive-oot.txt";
		const char *fn = "bin/exhaustive-mm.txt";
		struct File *file = FileFromFilename(fn);
		char *src = file->data;
		char *tok;
		for (;;) {
			tok = strtok(src, "\r\n");
			if (!tok) break;
			src = 0;
			fprintf(stderr, "'%s'\n", tok);
			scene = SceneFromFilenamePredictRooms(tok);
			/*
			if (scene->rooms[0].headers[0].meshFormat == 1
				&& scene->rooms[0].headers[0].image.base.amountType
					== ROOM_SHAPE_IMAGE_AMOUNT_MULTI
			) Die("found ROOM_SHAPE_IMAGE_AMOUNT_MULTI");
			*/
			SceneToFilename(scene, 0);
			SceneFree(scene);
		}
		FileFree(file);
		fprintf(stderr, "successfully wrote everything\n");
		SceneWriterCleanup();
		return 0;
	}
#endif
	
	ExePath(argv[0]);
	
	if (argc == 2)
	{
		scene = SceneFromFilenamePredictRooms(argv[1]);
		
		// test textures
		if (false)
		{
			struct File *file = FileFromFilename("bin/test.bin");
			
			DataBlobSegmentSetup(6, file->data, file->dataEnd, 0);
			
			DataBlobSegmentsPopulateFromMeshNew(0x06000000, 0);
			
			sb_array(struct TextureBlob, texBlobs) = 0;
			TextureBlobSbArrayFromDataBlobs(file, DataBlobSegmentGetHead(6), &texBlobs);
			scene->textureBlobs = texBlobs;
		}
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
	
	SceneWriterCleanup();
	return 0;
}

