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

struct CutsceneOot;

#include "file.h"
#include "texanim.h"
#include "stretchy_buffer.h"
#include "datablobs.h"
#include "extmath.h"
#include "skelanime.h"
#include <stdint.h>
#include <stdbool.h>

#if 1 /* region: macros */
#define UNFOLD_RGB(v)   (v).r, (v).g, (v).b
#define UNFOLD_RGBA(v)  (v).r, (v).g, (v).b, (v).a
#define UNFOLD_VEC3(v)  (v).x, (v).y, (v).z
#define UNFOLD_VEC2(v)  (v).x, (v).y

#define UNFOLD_RGB_EXT(v, action)   (v).r action, (v).g action, (v).b action

#define UNFOLD_VEC3_EXT(v, action)   (v).x action, (v).y action, (v).z action

#define UNFOLD_ARRAY_3(TYPE, ADDR) ((TYPE*)ADDR)[0], ((TYPE*)ADDR)[1], ((TYPE*)ADDR)[2]

#define MAX(A, B) (((A) > (B)) ? (A) : (B))
#define MAX3(A, B, C) MAX(A, MAX(B, C))

#define MIN(A, B) (((A) < (B)) ? (A) : (B))

#define CODE_AS_STRING(...) # __VA_ARGS__

#define INSTANCE_PREV_DEFAULT 0x10000
#define INSTANCE_POS_PREV_INIT (Vec3f){ FLT_MAX, FLT_MAX, FLT_MAX }
#define INSTANCE_PREV_INIT { \
	INSTANCE_PREV_DEFAULT, \
	INSTANCE_PREV_DEFAULT, \
	INSTANCE_PREV_DEFAULT, \
	INSTANCE_PREV_DEFAULT, \
	INSTANCE_POS_PREV_INIT \
}

#define for_in(index, count) for (int index = 0; index < count; ++index)

#define ON_CHANGE(VARIABLE) \
	static typeof(VARIABLE) onchange##__LINE__; \
	if (onchange##__LINE__ != (VARIABLE) && ((onchange##__LINE__ = (VARIABLE)) == (VARIABLE)))

#define u8r3(X)  {  u8r(X),  u8r(((const uint8_t*)(X)) + 1),  u8r(((const uint8_t*)(X)) + 1) }
#define u16r3(X) { u16r(X), u16r(((const uint8_t*)(X)) + 2), u16r(((const uint8_t*)(X)) + 4) }
#define u32r3(X) { u32r(X), u32r(((const uint8_t*)(X)) + 4), u32r(((const uint8_t*)(X)) + 8) }
#define f32r3(X) { f32r(X), f32r(((const uint8_t*)(X)) + 4), f32r(((const uint8_t*)(X)) + 8) }
#define s16r3(X) { s16r(X), s16r(((const uint8_t*)(X)) + 2), s16r(((const uint8_t*)(X)) + 4) }
float f32r(const void *data);
uint32_t f32tou32(float v);

#endif /* macros */

#if 1 // region: enum strings

// https://gist.github.com/linneman/99ff9ff86d7b4c69604b012bfcc4c258

#define ENUM_BODY(name, value) \
	name value,

#define AS_STRING_CASE(name, value) \
	case name: return #name;

#define DEFINE_ENUM(name, list) \
	typedef enum { \
		list(ENUM_BODY) \
	}name;

#define AS_STRING_DEC(name, list) \
	const char* name##AsString(name n);

#define AS_STRING_FUNC(name, list) \
	const char* name##AsString(name n) { \
		switch (n) { \
			list(AS_STRING_CASE) \
			default: \
			{ \
				static char int_as_string[8];\
				snprintf( int_as_string, sizeof(int_as_string), "%d", n ); \
				return int_as_string; \
			} \
		} \
	}

#endif // endregion

#if 1 /* region: types */

struct File;
struct Room;
struct Object;
struct SceneHeader;
struct Scene;
struct RoomMeshSimple;

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

typedef struct {
	/* 0x0 */ s16 setting; // camera setting described by CameraSettingType enum
	///* 0x2 */ s16 count; // sb_count() the array
	/* 0x4 */ sb_array(Vec3s, actorCsCamFuncData); // s16 data grouped in threes
} ActorCsCamInfo; // size = 0x8

typedef struct {
	/* 0x00 */ s16 priority; // Lower means higher priority. -1 means it ignores priority
	/* 0x02 */ s16 length;
	/* 0x04 */ s16 csCamId; // Index of CsCameraEntry to use. Negative indices use sGlobalCamDataSettings. Indices 0 and above use CsCameraEntry from a sceneLayer
	/* 0x06 */ s16 scriptIndex;
	/* 0x08 */ s16 additionalCsId;
	/* 0x0A */ u8 endSfx;
	/* 0x0B */ u8 customValue; // 0 - 99: actor-specific custom value. 100+: spawn. 255: none
	/* 0x0C */ s16 hudVisibility;
	/* 0x0E */ u8 endCam;
	/* 0x0F */ u8 letterboxSize;
} ActorCutscene; // size = 0x10

#if 1 // region: collision types
typedef struct {
	uint32_t w0;
	uint32_t w1;
} SurfaceType;

// The structure used for all instances of int16_t data from `BgCamInfo` with the exception of crawlspaces.
// See `Camera_Subj4` for Vec3s data usage in crawlspaces
/**
 * Crawlspaces
 * Moves the camera from third person to first person when entering a crawlspace
 * While in the crawlspace, link remains fixed in a single direction
 * The camera is what swings up and down while crawling forward or backwards
 *
 * Note:
 * Subject 4 uses bgCamFuncData.data differently than other functions:
 * All Vec3s data are points along the crawlspace
 * The second point represents the entrance, and the second to last point represents the exit
 * All other points are unused
 * All instances of crawlspaces have 6 points, except for the Testroom scene which has 9 points
 */
typedef struct {
	Vec3s pos;
	Vec3s rot;
	int16_t fov;
	union {
		int16_t roomImageOverrideBgCamIndex;
		int16_t timer;
		int16_t flags;
	};
	int16_t unused;
} BgCamFuncData; // size = 0x12

typedef struct {
	uint16_t setting; // camera setting described by CameraSettingType enum
	int16_t count; // only used when `bgCamFuncData` is a list of points used for crawlspaces
	BgCamFuncData *data; // if crawlspace, is array of Vec3s
	uint32_t dataAddr;
	const void *dataAddrResolved;
} BgCamInfo; // size = 0x8

typedef struct
{
	uint16_t type;
	uint16_t vtxData[3];
		///* vtxData[0] */ uint16_t flags_vIA; // 0xE000 is poly exclusion flags (xpFlags), 0x1FFF is vtxId
		///* vtxData[1] */ uint16_t flags_vIB; // 0xE000 is flags, 0x1FFF is vtxId
		///////////////////////////////////////// 0x2000 = poly IsFloorConveyor surface
		///* vtxData[2] */ uint16_t vIC;
	Vec3s normal; // Unit normal vector
	//////////////// Value ranges from -0x7FFF to 0x7FFF, representing -1.0 to 1.0; 0x8000 is invalid
	int16_t dist; // Plane distance from origin along the normal
} CollisionPoly; // size = 0x10

typedef struct {
	/* 0x00 */ int16_t xMin;
	/* 0x02 */ int16_t ySurface;
	/* 0x04 */ int16_t zMin;
	/* 0x06 */ int16_t xLength;
	/* 0x08 */ int16_t zLength;
	/* 0x0A */ uint16_t unused; // these bytes are sometimes non-zero in MM
	/* 0x0C */ uint32_t properties;
} WaterBox; // size = 0x10

typedef struct {
	/* 0x00 */ Vec3s minBounds; // minimum coordinates of poly bounding box
	/* 0x06 */ Vec3s maxBounds; // maximum coordinates of poly bounding box
	/* 0x0C */ uint16_t numVertices;
	/* 0x10 */ Vec3s* vtxList;
	/* 0x14 */ uint16_t numPolygons;
	/* 0x18 */ CollisionPoly* polyList;
	/* 0x1C */ SurfaceType* surfaceTypeList;
	/* 0x20 */ sb_array(BgCamInfo, bgCamList);
	/* 0x24 */ uint16_t numWaterBoxes;
	/* 0x28 */ WaterBox* waterBoxes; // TODO make sb_array later so it can be resized
	
	uint32_t originalSegmentAddress;
	int numSurfaceTypes;
	int numExits;
	int numCameras;
} CollisionHeader; // original name: BGDataInfo
#endif // endregion

#if 1 // region: prerendered background types

typedef struct {
	/* 0x04 */ u32   source;
	/* 0x08 */ u32   unk_0C;
	/* 0x0C */ u32   tlut;
	/* 0x10 */ u16   width;
	/* 0x12 */ u16   height;
	/* 0x14 */ u8    fmt;
	/* 0x15 */ u8    siz;
	/* 0x16 */ u16   tlutMode;
	/* 0x18 */ u16   tlutCount;
	// extras
	uint32_t sourceBEU32;
	uint32_t tlutBEU32;
} RoomShapeImage;

typedef struct {
	/* 0x00 */ u16   unk_00;
	/* 0x02 */ u8    bgCamIndex; // for which bg cam index is this entry for
	RoomShapeImage   image;
} RoomShapeImageMultiBgEntry; // size = 0x1C

typedef enum {
	/* 1 */ ROOM_SHAPE_IMAGE_AMOUNT_SINGLE = 1,
	/* 2 */ ROOM_SHAPE_IMAGE_AMOUNT_MULTI
} RoomShapeImageAmountType;

typedef struct {
	///* 0x00 */ RoomShapeBase base; // handled elsewhere
	/* 0x01 */ u8 amountType; // RoomShapeImageAmountType
	///* 0x04 */ sb_array(struct RoomMeshSimple, entry); // handled elsewhere
} RoomShapeImageBase; // size = 0x08

typedef struct {
	/* 0x00 */ RoomShapeImageBase base;
	RoomShapeImage   image;
} RoomShapeImageSingle; // size = 0x20

typedef struct {
	/* 0x00 */ RoomShapeImageBase base;
	/* 0x08 */ u8 numBackgrounds;
	/* 0x0C */ sb_array(RoomShapeImageMultiBgEntry, backgrounds);
} RoomShapeImageMulti; // size = 0x10

#endif // endregion

typedef enum ActorCategory {
	/* 0x00 */ ACTORCAT_SWITCH,
	/* 0x01 */ ACTORCAT_BG,
	/* 0x02 */ ACTORCAT_PLAYER,
	/* 0x03 */ ACTORCAT_EXPLOSIVE,
	/* 0x04 */ ACTORCAT_NPC,
	/* 0x05 */ ACTORCAT_ENEMY,
	/* 0x06 */ ACTORCAT_PROP,
	/* 0x07 */ ACTORCAT_ITEMACTION,
	/* 0x08 */ ACTORCAT_MISC,
	/* 0x09 */ ACTORCAT_BOSS,
	/* 0x0A */ ACTORCAT_DOOR,
	/* 0x0B */ ACTORCAT_CHEST,
	/* 0x0C */ ACTORCAT_MAX
} ActorCategory;

enum InstanceTab
{
	INSTANCE_TAB_ACTOR = 0,
	INSTANCE_TAB_DOOR,
	INSTANCE_TAB_SPAWN,
	INSTANCE_TAB_COUNT,
};

struct Instance
{
	uint16_t  id;
	uint16_t  xrot;
	uint16_t  yrot;
	uint16_t  zrot;
	uint16_t  params;
	
	Vec3f     pos;
	uint8_t   tab; // enum InstanceTab
	
	struct {
		uint8_t   frontRoom;
		uint8_t   frontCamera;
		uint8_t   backRoom;
		uint8_t   backCamera;
	} doorway;
	
	struct {
		uint8_t room;
	} spawnpoint;
	
	SkelAnime skelanime;
	
	// for tracking changes
	struct {
		uint32_t xrot;
		uint32_t yrot;
		uint32_t zrot;
		uint32_t params;
		Vec3f    pos;
	} prev;
};

struct RoomMeshSimple
{
	Vec3f    center;
	int16_t  radius;
	uint32_t opa;
	uint32_t xlu;
	uint32_t opaBEU32; // DataBlobApply() updates this
	uint32_t xluBEU32;
};

struct RoomHeader
{
	struct Room *room;
	sb_array(struct Instance, instances);
	sb_array(uint16_t, objects);
	sb_array(struct RoomMeshSimple, displayLists);
	sb_array(uint32_t, unhandledCommands);
	uint32_t addr;
	uint8_t meshFormat;
	bool isBlank;
	union {
		RoomShapeImageBase    base;
		RoomShapeImageSingle  single;
		RoomShapeImageMulti   multi;
	} image;
};

struct Room
{
	struct File *file;
	struct Scene *scene;
	struct DataBlob *blobs;
	sb_array(struct RoomHeader, headers);
};

struct ActorPath
{
	//uint8_t numPoints; // unused b/c sb_array() has sb_count()
	uint8_t additionalPathIndex; // mm only
	uint16_t customValue; // mm only
	sb_array(Vec3f, points);
};

struct SceneHeader
{
	struct Scene *scene;
	sb_array(struct Instance, spawns);
	sb_array(ZeldaLight, lights);
	sb_array(uint32_t, unhandledCommands);
	sb_array(struct ActorPath, paths);
	sb_array(struct Instance, doorways);
	struct CutsceneOot *cutsceneOot;
	sb_array(struct CutsceneListMm, cutsceneListMm);
	sb_array(ActorCsCamInfo, actorCsCamInfo);
	sb_array(ActorCutscene, actorCutscenes);
	uint32_t addr;
	int numRooms;
	struct {
		int sceneSetupType;
		sb_array(AnimatedMaterial, sceneSetupData);
	} mm;
	sb_array(uint16_t, exits); // TODO if == 0, fall back to header[0] exit data
	uint32_t exitsSegAddr;
	bool isBlank;
};

struct Scene
{
	struct File *file;
	struct DataBlob *blobs;
	sb_array(struct TextureBlob, textureBlobs);
	sb_array(struct Room, rooms);
	sb_array(struct SceneHeader, headers);
	CollisionHeader *collisions;
	
	int test;
};
#endif /* types */

#if 1 /* region: function prototypes */

struct Scene *SceneFromFilename(const char *filename);
struct Scene *SceneFromFilenamePredictRooms(const char *filename);
void SceneToFilename(struct Scene *scene, const char *filename);
struct Room *RoomFromFilename(const char *filename);
void ScenePopulateRoom(struct Scene *scene, int index, struct Room *room);
void SceneReadyDataBlobs(struct Scene *scene);
void Die(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
const char *QuickFmt(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void *Calloc(size_t howMany, size_t sizeEach);
void SceneAddHeader(struct Scene *scene, struct SceneHeader *header);
void RoomAddHeader(struct Room *room, struct RoomHeader *header);
void SceneAddRoom(struct Scene *scene, struct Room *room);
void SceneFree(struct Scene *scene);
void RoomFree(struct Room *room);
char *Strdup(const char *str);
char *StrdupPad(const char *str, int padding);
void StrcatCharLimit(char *dst, unsigned int codepoint, unsigned int dstByteSize);
char *StrToLower(char *str);
void StrRemoveChar(char *charAt);
void *MemmemAligned(const void *haystack, size_t haystackLen, const void *needle, size_t needleLen, size_t byteAlignment);
void *Memmem(const void *haystack, size_t haystackLen, const void *needle, size_t needleLen);
const char *ExePath(const char *path);
struct DataBlob *MiscSkeletonDataBlobs(struct File *file, struct DataBlob *head, uint32_t segAddr);
void TextureBlobSbArrayFromDataBlobs(struct File *file, struct DataBlob *head, struct TextureBlob **texBlobs);
void SceneWriterCleanup(void);

struct Instance *InstanceAddToListGeneric(struct Instance **list, const void *src);
void InstanceDeleteFromListGeneric(struct Instance **list, const void *src);

struct Scene *WindowOpenFile(void);
struct Scene *WindowLoadScene(const char *fn);
void WindowSaveScene(void);
void WindowSaveSceneAs(void);

CollisionHeader *CollisionHeaderNewFromSegment(uint32_t segAddr);
void CollisionHeaderFree(CollisionHeader *header);

#endif /* function prototypes */

#ifdef __cplusplus
}
#endif

#endif /* Z64SCENE_MISC_H_INCLUDED */

