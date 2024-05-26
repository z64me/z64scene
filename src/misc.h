//
// misc.h
//
// misc things
//

#ifndef Z64SCENE_MISC_H_INCLUDED
#define Z64SCENE_MISC_H_INCLUDED

#include "stretchy_buffer.h"
#include <stdint.h>

#if 1 /* region: types */

struct File;
struct Room;
struct Object;
struct SceneHeader;
struct Scene;

struct File
{
	void *data;
	void *dataEnd;
	char *filename;
	size_t size;
};

struct Instance
{
	uint16_t  id;
	int16_t   x;
	int16_t   y;
	int16_t   z;
	int16_t   xrot;
	int16_t   yrot;
	int16_t   zrot;
	uint16_t  params;
};

struct RoomHeader
{
	struct Room *room;
	sb_array(struct Instance, instances);
	sb_array(uint16_t, objects);
	uint32_t addr;
};

struct Room
{
	struct File *file;
	struct Scene *scene;
	sb_array(struct RoomHeader, headers);
};

struct Object
{
	struct File *file;
};

struct SpawnPoint
{
	struct Instance inst;
	uint8_t room;
};

struct SceneHeader
{
	struct Scene *scene;
	sb_array(struct SpawnPoint, spawns);
	uint32_t addr;
	int numRooms;
};

struct Scene
{
	struct File *file;
	sb_array(struct Room, rooms);
	sb_array(struct SceneHeader, headers);
};
#endif /* types */

#if 1 /* region: function prototypes */

struct File *FileFromFilename(const char *filename);
struct Scene *SceneFromFilename(const char *filename);
struct Room *RoomFromFilename(const char *filename);
struct Object *ObjectFromFilename(const char *filename);
void ScenePopulateRoom(struct Scene *scene, int index, struct Room *room);
void Die(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void *Calloc(size_t howMany, size_t sizeEach);
void SceneAddHeader(struct Scene *scene, struct SceneHeader *header);
void RoomAddHeader(struct Room *room, struct RoomHeader *header);
void SceneAddRoom(struct Scene *scene, struct Room *room);
void FileFree(struct File *file);
void SceneFree(struct Scene *scene);
void ObjectFree(struct Object *object);
void RoomFree(struct Room *room);

#endif /* function prototypes */

#endif /* Z64SCENE_MISC_H_INCLUDED */

