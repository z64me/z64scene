//
// misc.h
//
// misc things
//

#ifndef Z64SCENE_MISC_H_INCLUDED
#define Z64SCENE_MISC_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "stretchy_buffer.h"
#include <stdint.h>

#if 1 /* region: macros */
#define UNFOLD_RGB(v)   (v).r, (v).g, (v).b
#define UNFOLD_RGBA(v)  (v).r, (v).g, (v).b, (v).a
#define UNFOLD_VEC3(v)  (v).x, (v).y, (v).z

#define UNFOLD_RGB_EXT(v, action)   (v).r action, (v).g action, (v).b action

#define UNFOLD_ARRAY_3(TYPE, ADDR) ((TYPE*)ADDR)[0], ((TYPE*)ADDR)[1], ((TYPE*)ADDR)[2]

#define Vec3_Substract(dest, a, b) \
	dest.x = a.x - b.x; \
	dest.y = a.y - b.y; \
	dest.z = a.z - b.z

#define SQ(x)        ((x) * (x))
#define Math_SinS(x) sinf(BinToRad((int16_t)(x)))
#define Math_CosS(x) cosf(BinToRad((int16_t)(x)))
#define Math_SinF(x) sinf(DegToRad(x))
#define Math_CosF(x) cosf(DegToRad(x))
#define Math_SinR(x) sinf(x)
#define Math_CosR(x) cosf(x)

// trigonometry macros
#define DegToBin(degreesf) (int16_t)(degreesf * 182.04167f + .5f)
#define RadToBin(radf)     (int16_t)(radf * (32768.0f / M_PI))
#define RadToDeg(radf)     (radf * (180.0f / M_PI))
#define DegToRad(degf)     (degf * (M_PI / 180.0f))
#define BinFlip(angle)     ((int16_t)(angle - 0x7FFF))
#define BinSub(a, b)       ((int16_t)(a - b))
#define BinToDeg(binang)   ((float)binang * (360.0001525f / 65535.0f))
#define BinToRad(binang)   (((float)binang / 32768.0f) * M_PI)

#endif /* macros */

#if 1 /* region: types */

struct File;
struct Room;
struct Object;
struct SceneHeader;
struct Scene;

typedef struct ZeldaRGB {
	uint8_t r, g, b;
} ZeldaRGB;

typedef struct ZeldaVecS8 {
	int8_t x, y, z;
} ZeldaVecS8;

typedef struct ZeldaLight {
	ZeldaRGB ambient;
	ZeldaVecS8 diffuse_a_dir;
	ZeldaRGB diffuse_a;
	ZeldaVecS8 diffuse_b_dir;
	ZeldaRGB diffuse_b;
	ZeldaRGB fog;
	uint16_t fog_near;
	uint16_t fog_far;
} ZeldaLight;

// another way to interface with a ZeldaLight
typedef struct {
	/* 0x00 */ uint8_t ambientColor[3];
	/* 0x03 */ int8_t light1Dir[3];
	/* 0x06 */ uint8_t light1Color[3];
	/* 0x09 */ int8_t light2Dir[3];
	/* 0x0C */ uint8_t light2Color[3];
	/* 0x0F */ uint8_t fogColor[3];
	/* 0x12 */ int16_t fogNear;
	/* 0x14 */ int16_t fogFar;
} EnvLightSettings; // size = 0x16

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

struct RoomMeshSimple
{
	uint32_t opa;
	uint32_t xlu;
};

struct RoomHeader
{
	struct Room *room;
	sb_array(struct Instance, instances);
	sb_array(uint16_t, objects);
	sb_array(struct RoomMeshSimple, displayLists);
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
	sb_array(ZeldaLight, lights);
	uint32_t addr;
	int numRooms;
};

struct Scene
{
	struct File *file;
	sb_array(struct Room, rooms);
	sb_array(struct SceneHeader, headers);
	int test;
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

#ifdef __cplusplus
}
#endif

#endif /* Z64SCENE_MISC_H_INCLUDED */

