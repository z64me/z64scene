//
// misc.h
//
// misc things
//

#include "misc.h"
#include "stretchy_buffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
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
		Die("memory error on Calloc(%zu, %zu)\n", howMany, sizeEach);
	
	return result;
}

char *Strdup(const char *str)
{
	char *result = Calloc(1, strlen(str) + 1);
	
	strcpy(result, str);
	
	return result;
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
	
	return result;
}

struct Scene *SceneFromFilename(const char *filename)
{
	struct Scene *result = Calloc(1, sizeof(*result));
	
	result->file = FileFromFilename(filename);
	
	return private_SceneParseAfterLoad(result);
}

void SceneAddHeader(struct Scene *scene, struct SceneHeader *header)
{
	header->scene = scene;
	
	sb_push(scene->headers, *header);
	
	free(header);
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
	// TODO make u16r and friends accept const, just cast here for now
	/*const*/ uint8_t *data8 = (uint8_t*)data;
	
	return (struct Instance) {
		.id = u16r(data8 + 0)
		, .x = s16r(data8 + 2)
		, .y = s16r(data8 + 4)
		, .z = s16r(data8 + 6)
		, .xrot = s16r(data8 + 8)
		, .yrot = s16r(data8 + 10)
		, .zrot = s16r(data8 + 12)
		, .params = u16r(data8 + 14)
	};
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
				result->refLights = (void*)(data8 + (u32r(walk + 4) & 0x00ffffff));
				result->numRefLights = walk[1];
				break;
			
			case 0x18: { // alternate headers
				altHeadersArray = data8 + (u32r(walk + 4) & 0x00ffffff);
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

