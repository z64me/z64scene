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

#include "texanim.h"
#include "stretchy_buffer.h"
#include "datablobs.h"
#include "extmath.h"
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
	/* 0x20 */ BgCamInfo* bgCamList; // TODO make sb_array later so it can be resized
	/* 0x24 */ uint16_t numWaterBoxes;
	/* 0x28 */ WaterBox* waterBoxes; // TODO make sb_array later so it can be resized
	
	uint32_t originalSegmentAddress;
	int numSurfaceTypes;
	int numExits;
	int numCameras;
} CollisionHeader; // original name: BGDataInfo
#endif // endregion

struct File
{
	void *data;
	void *dataEnd;
	char *filename;
	char *shortname;
	size_t size;
};

struct Instance
{
	uint16_t  id;
	int16_t   x;
	int16_t   y;
	int16_t   z;
	uint16_t  xrot;
	uint16_t  yrot;
	uint16_t  zrot;
	uint16_t  params;
	
	Vec3f     pos;
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
};

struct Room
{
	struct File *file;
	struct Scene *scene;
	struct DataBlob *blobs;
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

struct ActorPath
{
	sb_array(Vec3f, points);
};

struct Doorway
{
	uint8_t   frontRoom;
	uint8_t   frontCamera;
	uint8_t   backRoom;
	uint8_t   backCamera;
	uint16_t  id;
	int16_t   x;
	int16_t   y;
	int16_t   z;
	uint16_t  rot;
	uint16_t  params;
	
	Vec3f     pos;
};

struct SceneHeader
{
	struct Scene *scene;
	sb_array(struct SpawnPoint, spawns);
	sb_array(ZeldaLight, lights);
	sb_array(uint32_t, unhandledCommands);
	sb_array(struct ActorPath, paths);
	sb_array(struct Doorway, doorways);
	uint32_t addr;
	int numRooms;
	struct {
		int sceneSetupType;
		sb_array(AnimatedMaterial, sceneSetupData);
	} mm;
	bool isBlank;
};

struct Scene
{
	struct File *file;
	struct DataBlob *blobs;
	sb_array(struct TextureBlob, textureBlobs);
	sb_array(struct Room, rooms);
	sb_array(struct SceneHeader, headers);
	sb_array(uint16_t, exits);
	CollisionHeader *collisions;
	
	uint32_t exitsSegAddr;
	int test;
};
#endif /* types */

#if 1 /* region: function prototypes */

bool FileExists(const char *filename);
struct File *FileNew(const char *filename, size_t size);
struct File *FileFromFilename(const char *filename);
int FileToFilename(struct File *file, const char *filename);
const char *FileGetError(void);
struct Scene *SceneFromFilename(const char *filename);
struct Scene *SceneFromFilenamePredictRooms(const char *filename);
void SceneToFilename(struct Scene *scene, const char *filename);
struct Room *RoomFromFilename(const char *filename);
struct Object *ObjectFromFilename(const char *filename);
void ScenePopulateRoom(struct Scene *scene, int index, struct Room *room);
void SceneReadyDataBlobs(struct Scene *scene);
void Die(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void *Calloc(size_t howMany, size_t sizeEach);
void SceneAddHeader(struct Scene *scene, struct SceneHeader *header);
void RoomAddHeader(struct Room *room, struct RoomHeader *header);
void SceneAddRoom(struct Scene *scene, struct Room *room);
void FileFree(struct File *file);
void SceneFree(struct Scene *scene);
void ObjectFree(struct Object *object);
void RoomFree(struct Room *room);
char *Strdup(const char *str);
char *StrdupPad(const char *str, int padding);
void StrcatCharLimit(char *dst, unsigned int codepoint, unsigned int dstByteSize);
char *StrToLower(char *str);
void StrRemoveChar(char *charAt);
void *Memmem(const void *haystack, size_t haystackLen, const void *needle, size_t needleLen);
const char *ExePath(const char *path);
struct DataBlob *MiscSkeletonDataBlobs(struct File *file, struct DataBlob *head, uint32_t segAddr);
void TextureBlobSbArrayFromDataBlobs(struct File *file, struct DataBlob *head, struct TextureBlob **texBlobs);

struct Scene *WindowOpenFile(void);
struct Scene *WindowLoadScene(const char *fn);
void WindowSaveScene(void);
void WindowSaveSceneAs(void);

CollisionHeader *CollisionHeaderNewFromSegment(uint32_t segAddr);

#endif /* function prototypes */

#ifdef __cplusplus
}
#endif

#endif /* Z64SCENE_MISC_H_INCLUDED */

