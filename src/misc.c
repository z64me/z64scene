//
// misc.h
//
// misc things
//

#include "misc.h"
#include "stretchy_buffer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <inttypes.h>
#include <bigendian.h>

#if 1 /* region: private function declarations */

static struct Scene *private_SceneParseAfterLoad(struct Scene *scene);
static void private_SceneParseAddHeader(struct Scene *scene, uint32_t addr);
static struct Room *private_RoomParseAfterLoad(struct Room *room);
static void private_RoomParseAddHeader(struct Room *room, uint32_t addr);
static struct Instance private_InstanceParse(const void *data);

#endif /* private function declarations */

void Die(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	
	vfprintf(stderr, fmt, args);
	
	va_end(args);
	fprintf(stderr, "\n");
	
	exit(EXIT_FAILURE);
}

void *Calloc(size_t howMany, size_t sizeEach)
{
	void *result = calloc(howMany, sizeEach);
	
	if (!result)
		Die("memory error on Calloc(%"PRIuPTR", %"PRIuPTR")\n", howMany, sizeEach);
	
	return result;
}

char *StrdupPad(const char *str, int padding)
{
	char *result = Calloc(1, strlen(str) + padding + 1);
	
	strcpy(result, str);
	
	return result;
}

char *Strdup(const char *str)
{
	char *result = Calloc(1, strlen(str) + 1);
	
	strcpy(result, str);
	
	return result;
}

void StrToLower(char *str)
{
	if (!str)
		return;
	
	for (char *c = str; *c; ++c)
		*c = tolower(*c);
}

void StrRemoveChar(char *charAt)
{
	if (!charAt)
		return;
	
	for ( ; charAt[0]; ++charAt)
		charAt[0] = charAt[1];
}

void StrcatCharLimit(char *dst, unsigned int codepoint, unsigned int dstByteSize)
{
	unsigned int end = strlen(dst);
	
	if (end >= dstByteSize - 2)
		return;
	
	dst[end] = codepoint;
	dst[end + 1] = '\0';
}

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#elif __linux__
#include <unistd.h>
#include <limits.h>
#endif
const char *ExePath(const char *path)
{
	static char *basePath = 0;
	static char *appendTo;
	
	if (!basePath)
	{
	#ifdef _WIN32
		basePath = malloc(MAX_PATH);
		GetModuleFileName(NULL, basePath, MAX_PATH);
	#elif __linux__
		basePath = malloc(PATH_MAX);
		ssize_t count = readlink("/proc/self/exe", basePath, PATH_MAX);
		if (count == -1)
			Die("readlink() fatal error");
	#else
		#error please implement ExePath() for this platform
	#endif
		char *slash = strrchr(basePath, '/');
		char *slash2 = strrchr(basePath, '\\');
		
		appendTo = MAX(slash, slash2);
		if (!appendTo)
			Die("TODO get current working directory instead");
		++appendTo;
		
		*appendTo = '\0';
		fprintf(stderr, "ExePath = '%s'\n", basePath);
	}
	else
		strcpy(appendTo, path);
	
	return basePath;
}

bool FileExists(const char *filename)
{
	FILE *fp = fopen(filename, "rb");
	
	if (!fp)
		return false;
	
	fclose(fp);
	
	return true;
}

struct File *FileFromFilename(const char *filename)
{
	struct File *result = Calloc(1, sizeof(*result));
	FILE *fp;
	
	if (!filename || !*filename) Die("empty filename");
	if (!(fp = fopen(filename, "rb"))) Die("failed to open '%s' for reading", filename);
	if (fseek(fp, 0, SEEK_END)) Die("error moving read head '%s'", filename);
	if (!(result->size = ftell(fp))) Die("error reading '%s', empty?", filename);
	if (fseek(fp, 0, SEEK_SET)) Die("error moving read head '%s'", filename);
	result->data = Calloc(1, result->size);
	result->dataEnd = ((uint8_t*)result->data) + result->size;
	if (fread(result->data, 1, result->size, fp) != result->size)
		Die("error reading contents of '%s'", filename);
	if (fclose(fp)) Die("error closing file '%s' after reading", filename);
	result->filename = Strdup(filename);
	result->shortname = Strdup(MAX3(
		strrchr(filename, '/') + 1
		, strrchr(filename, '\\') + 1
		, filename
	));
	
	return result;
}

struct Scene *SceneFromFilename(const char *filename)
{
	struct Scene *result = Calloc(1, sizeof(*result));
	
	result->file = FileFromFilename(filename);
	
	return private_SceneParseAfterLoad(result);
}

struct Scene *SceneFromFilenamePredictRooms(const char *filename)
{
	struct Scene *scene = SceneFromFilename(filename);
	char *roomNameBuf = StrdupPad(filename, 100);
	char *lastSlash = MAX(strrchr(roomNameBuf, '\\'), strrchr(roomNameBuf, '/'));
	if (!lastSlash)
		lastSlash = roomNameBuf;
	else
		lastSlash += 1;
	lastSlash = MAX(lastSlash, strstr(lastSlash, "_scene"));
	if (*lastSlash == '_')
		lastSlash += 1;
	
	for (int i = 0; i < scene->headers[0].numRooms; ++i)
	{
		const char *variations[] = {
			"room_%d.zmap",
			"room_%02d.zmap",
			"room_%d.zroom",
			"room_%02d.zroom"
		};
		bool found = false;
		
		for (int k = 0; k < sizeof(variations) / sizeof(*variations); ++k)
		{
			sprintf(lastSlash, variations[k], i);
			
			fprintf(stderr, "%s\n", roomNameBuf);
			
			if (FileExists(roomNameBuf))
			{
				SceneAddRoom(scene, RoomFromFilename(roomNameBuf));
				found = true;
				break;
			}
		}
		
		if (!found)
			Die("could not find room_%d", i);
	}
	
	// test this out
	SceneReadyDataBlobs(scene);
	
	free(roomNameBuf);
	return scene;
}

void SceneAddHeader(struct Scene *scene, struct SceneHeader *header)
{
	header->scene = scene;
	
	sb_push(scene->headers, *header);
	
	free(header);
}

void SceneReadyDataBlobs(struct Scene *scene)
{
	DataBlobSegmentSetup(2, scene->file->data, scene->file->dataEnd, scene->blobs);
	
	sb_foreach(scene->rooms, {
		
		DataBlobSegmentSetup(3, each->file->data, each->file->dataEnd, each->blobs);
		
		typeof(each->headers) headers = each->headers;
		
		sb_foreach(headers, {
			typeof(each->displayLists) dls = each->displayLists;
			
			sb_foreach(dls, {
				if (each->opa)
					DataBlobSegmentsPopulateFromMeshNew(each->opa);
				if (each->xlu)
					DataBlobSegmentsPopulateFromMeshNew(each->xlu);
			});
		});
		
		each->blobs = DataBlobSegmentGetHead(3);
		
		fprintf(stderr, "'%s' data blobs:\n", each->file->filename);
		DataBlobPrintAll(each->blobs);
	});
	
	scene->blobs = DataBlobSegmentGetHead(2);
	
	fprintf(stderr, "'%s' data blobs:\n", scene->file->filename);
	DataBlobPrintAll(scene->blobs);
	
	// generate texture blob list
	{
		TextureBlobSbArrayFromDataBlobs(scene->file, scene->blobs, &scene->textureBlobs);
		sb_foreach(scene->rooms, {
			TextureBlobSbArrayFromDataBlobs(each->file, each->blobs, &scene->textureBlobs);
		});
	}
	
	// test: create files w/ all data blobs zeroed
	if (false)
	{
		sb_foreach(scene->rooms, {
			
			for (struct DataBlob *blob = each->blobs; blob; blob = blob->next)
				memset((void*)blob->refData, 0, blob->sizeBytes);
			
			char tmp[1024];
			struct File *file = each->file;
			
			sprintf(tmp, "%s.test", file->filename);
			
			FILE *fp = fopen(tmp, "wb");
			if (!fp) continue;
			fwrite(file->data, 1, file->size, fp);
			fclose(fp);
		});
		
		exit(0);
	}
	
	fprintf(stderr, "total texture blobs %d\n", sb_count(scene->textureBlobs));
}

void TextureBlobSbArrayFromDataBlobs(struct File *file, struct DataBlob *head, struct TextureBlob **texBlobs)
{
	for (struct DataBlob *blob = head; blob; blob = blob->next)
	{
		if (blob->type == DATA_BLOB_TYPE_TEXTURE)
			sb_push(*texBlobs, TextureBlobStack(blob, file));
	};
}

struct DataBlob *MiscSkeletonDataBlobs(struct File *file, struct DataBlob *head, uint32_t segAddr)
{
	DataBlobSegmentSetup(6, file->data, file->dataEnd, head);
	
	// skeleton
	const uint8_t *sk = DataBlobSegmentAddressToRealAddress(segAddr);
	if (sk)
	{
		int numLimbs = sk[4];
		uint32_t limbTableSeg = u32r(sk);
		const uint8_t *limbTable = DataBlobSegmentAddressToRealAddress(limbTableSeg);
		
		for (int i = 0; i < numLimbs; ++i)
		{
			uint32_t limbSeg = u32r(limbTable + i * 4);
			const uint8_t *limb = DataBlobSegmentAddressToRealAddress(limbSeg);
			uint32_t dlist = u32r(limb + 8);
			
			DataBlobSegmentsPopulateFromMeshNew(dlist);
		}
	}
	
	return DataBlobSegmentGetHead(6);
}

void RoomAddHeader(struct Room *room, struct RoomHeader *header)
{
	header->room = room;
	
	sb_push(room->headers, *header);
	
	free(header);
}

void FileFree(struct File *file)
{
	if (!file)
		return;
	
	if (file->data)
		free(file->data);
	
	if (file->filename)
		free(file->filename);
	
	if (file->shortname)
		free(file->shortname);
	
	free(file);
}

void RoomHeaderFree(struct RoomHeader *header)
{
	sb_free(header->instances);
	sb_free(header->objects);
	sb_free(header->displayLists);
}

void RoomFree(struct Room *room)
{
	FileFree(room->file);
	
	sb_foreach(room->headers, {
		RoomHeaderFree(each);
	});
	sb_free(room->headers);
}

void SceneHeaderFree(struct SceneHeader *header)
{
	sb_free(header->spawns);
	sb_free(header->lights);
}

void SceneFree(struct Scene *scene)
{
	FileFree(scene->file);
	
	sb_foreach(scene->headers, {
		SceneHeaderFree(each);
	});
	sb_free(scene->headers);
	
	sb_foreach(scene->rooms, {
		RoomFree(each);
	});
	sb_free(scene->rooms);
	
	free(scene);
}

void ObjectFree(struct Object *object)
{
	FileFree(object->file);
	
	free(object);
}

void SceneAddRoom(struct Scene *scene, struct Room *room)
{
	room->scene = scene;
	
	sb_push(scene->rooms, *room);
	
	free(room);
}

struct Room *RoomFromFilename(const char *filename)
{
	struct Room *result = Calloc(1, sizeof(*result));
	
	result->file = FileFromFilename(filename);
	
	return private_RoomParseAfterLoad(result);
}

struct Object *ObjectFromFilename(const char *filename)
{
	struct Object *result = Calloc(1, sizeof(*result));
	
	result->file = FileFromFilename(filename);
	
	return result;
}

void ScenePopulateRoom(struct Scene *scene, int index, struct Room *room);


#if 1 /* region: private function implementations */

static struct Instance private_InstanceParse(const void *data)
{
	const uint8_t *data8 = data;
	
	return (struct Instance) {
		.id = u16r(data8 + 0)
		, .x = s16r(data8 + 2)
		, .y = s16r(data8 + 4)
		, .z = s16r(data8 + 6)
		, .xrot = s16r(data8 + 8)
		, .yrot = s16r(data8 + 10)
		, .zrot = s16r(data8 + 12)
		, .params = u16r(data8 + 14)
		, .pos = {
			s16r(data8 + 2)
			, s16r(data8 + 4)
			, s16r(data8 + 6)
		}
	};
}

static struct ZeldaLight private_ZeldaLightParse(const void *data)
{
	const uint8_t *data8 = data;
	
	return (struct ZeldaLight) {
		.ambient            = { UNFOLD_ARRAY_3(uint8_t, data8 +  0) }
		, .diffuse_a_dir    = { UNFOLD_ARRAY_3( int8_t, data8 +  3) }
		, .diffuse_a        = { UNFOLD_ARRAY_3(uint8_t, data8 +  6) }
		, .diffuse_b_dir    = { UNFOLD_ARRAY_3( int8_t, data8 +  9) }
		, .diffuse_b        = { UNFOLD_ARRAY_3(uint8_t, data8 + 12) }
		, .fog              = { UNFOLD_ARRAY_3(uint8_t, data8 + 15) }
		, .fog_near         = u16r(data8 + 18)
		, .fog_far          = u16r(data8 + 20)
	};
}

static bool private_IsLikelyHeader(uint32_t addr, uint8_t segment, uint32_t fileSize)
{
	if ((addr >> 24) != segment
		|| (addr & 7) // not 8 byte aligned
		|| (addr & 0x00ffffff) >= fileSize
	)
		return false;
	
	return true;
}

static void private_SceneParseAddHeader(struct Scene *scene, uint32_t addr)
{
	struct SceneHeader *result = Calloc(1, sizeof(*result));
	struct File *file = scene->file;
	uint8_t *data8 = file->data;
	uint8_t *walk = file->data;
	struct {
		uint8_t *positions;
		uint8_t *entrances;
		int count;
	} spawnPoints = {0};
	uint8_t *altHeadersArray = 0;
	result->mm.sceneSetupType = -1;
	
	// unlikely a scene header
	if (private_IsLikelyHeader(addr, 0x02, file->size) == false)
		addr = 0;
	
	// don't parse blank headers
	if (!(result->addr = addr))
	{
		SceneAddHeader(scene, result);
		return;
	}
	
	// walk the header
	for (walk += addr & 0x00ffffff; (void*)walk < file->dataEnd; walk += 8)
	{
		switch (*walk)
		{
			case 0x14: // end header
				walk = file->dataEnd;
				break;
			
			case 0x00: // spawn point positions
				spawnPoints.positions = data8 + (u32r(walk + 4) & 0x00ffffff);
				spawnPoints.count = walk[1]; // approximation, acceptable for now
				break;
			
			case 0x06: // spawn point entrances
				spawnPoints.entrances = data8 + (u32r(walk + 4) & 0x00ffffff);
				break;
			
			case 0x04: // room list
				result->numRooms = walk[1];
				break;
			
			case 0x0F: // light list
				uint8_t *arr = data8 + (u32r(walk + 4) & 0x00ffffff);
				
				for (int i = 0; i < walk[1]; ++i)
					sb_push(result->lights
						, private_ZeldaLightParse(arr + 0x16 * i)
					);
				break;
			
			case 0x18: { // alternate headers
				altHeadersArray = data8 + (u32r(walk + 4) & 0x00ffffff);
				break;
			}
			
			case 0x1A: { // mm texture animation
				result->mm.sceneSetupType = 1;
				result->mm.sceneSetupData = AnimatedMaterialNewFromSegment(u32r(walk + 4));
				sb_foreach(result->mm.sceneSetupData, {
					fprintf(stderr, "texanim[%d] type%d, seg%d\n", eachIndex, each->type, each->segment);
				});
				//Die("%d texanims\n", sb_count(result->mm.sceneSetupData));
				break;
			}
			
			default:
				break;
		}
	}
	
	// consolidate spawnPoints to friendlier format for viewing and editing
	if (spawnPoints.positions && spawnPoints.entrances)
	{
		for (int i = 0; i < spawnPoints.count; ++i)
		{
			struct SpawnPoint tmp = (struct SpawnPoint) {
				.room = spawnPoints.entrances[i * 2 + 1]
				, .inst = private_InstanceParse(spawnPoints.positions
					+ 16 * spawnPoints.entrances[i * 2]
				)
			};
			
			sb_push(result->spawns, tmp);
		}
	}
	
	// add after parsing
	SceneAddHeader(scene, result);
	
	// handler alternate headers
	if (altHeadersArray)
		for (int i = 0; i < 4; ++i)
			private_SceneParseAddHeader(scene, u32r(altHeadersArray + i * 4));
			
}

static void private_RoomParseAddHeader(struct Room *room, uint32_t addr)
{
	struct RoomHeader *result = Calloc(1, sizeof(*result));
	struct File *file = room->file;
	uint8_t *data8 = file->data;
	uint8_t *walk = file->data;
	uint8_t *altHeadersArray = 0;
	
	// unlikely a room header
	if (private_IsLikelyHeader(addr, 0x03, file->size) == false)
		addr = 0;
	
	// don't parse blank headers
	if (!(result->addr = addr))
	{
		RoomAddHeader(room, result);
		return;
	}
	
	// walk the header
	for (walk += addr & 0x00ffffff; (void*)walk < file->dataEnd; walk += 8)
	{
		switch (*walk)
		{
			case 0x14: // end header
				walk = file->dataEnd;
				break;
			
			case 0x01: { // instances
				uint8_t *arr = data8 + (u32r(walk + 4) & 0x00ffffff);
				
				for (int i = 0; i < walk[1]; ++i)
					sb_push(result->instances
						, private_InstanceParse(arr + 16 * i)
					);
				break;
			}
			
			case 0x0B: { // object id's
				uint8_t *arr = data8 + (u32r(walk + 4) & 0x00ffffff);
				
				for (int i = 0; i < walk[1]; ++i)
					sb_push(result->objects
						, u16r(arr + 2 * i)
					);
				break;
			}
			
			case 0x0A: { // mesh header
				uint8_t *d = data8 + (u32r(walk + 4) & 0x00ffffff);
				
				if (d[0] == 2)
				{
					int num = d[1];
					uint8_t *arr = data8 + (u32r(d + 4) & 0x00ffffff);
					
					for (int i = 0; i < num; ++i)
					{
						uint8_t *bin = arr + 16 * i;
						struct RoomMeshSimple tmp = {
							.opa = u32r(bin + 8)
							, .xlu = u32r(bin + 12)
							, .center = {
								s16r(bin + 0), s16r(bin + 2), s16r(bin + 4)
							}
							, .radius = s16r(bin + 6)
						};
						
						sb_push(result->displayLists, tmp);
					}
				}
				else if (d[0] == 0)
				{
					int num = d[1];
					uint8_t *arr = data8 + (u32r(d + 4) & 0x00ffffff);
					
					for (int i = 0; i < num; ++i)
					{
						uint8_t *bin = arr + 8 * i;
						struct RoomMeshSimple tmp = {
							.opa = u32r(bin + 0)
							, .xlu = u32r(bin + 4)
							, .radius = -1
						};
						
						sb_push(result->displayLists, tmp);
					}
				}
				else
				{
					fprintf(stderr, "unsupported mesh header type %d\n", d[0]);
				}
				break;
			}
			
			case 0x18: { // alternate headers
				altHeadersArray = data8 + (u32r(walk + 4) & 0x00ffffff);
				break;
			}
			
			default:
				break;
		}
	}
	
	// add after parsing
	RoomAddHeader(room, result);
	
	// handler alternate headers
	if (altHeadersArray)
		for (int i = 0; i < 4; ++i)
			private_RoomParseAddHeader(room, u32r(altHeadersArray + i * 4));
}

static struct Scene *private_SceneParseAfterLoad(struct Scene *scene)
{
	assert(scene);
	
	struct File *file = scene->file;
	
	assert(file);
	assert(file->data);
	assert(file->size);
	
	n64_segment_set(0x02, file->data);
	
	private_SceneParseAddHeader(scene, 0x02000000);
	
	return scene;
}

static struct Room *private_RoomParseAfterLoad(struct Room *room)
{
	assert(room);
	
	struct File *file = room->file;
	
	assert(file);
	assert(file->data);
	assert(file->size);
	
	private_RoomParseAddHeader(room, 0x03000000);
	
	return room;
}

#endif /* private functions */

