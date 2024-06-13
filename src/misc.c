//
// misc.h
//
// misc things
//

#include "misc.h"
#include "stretchy_buffer.h"
#include "cutscene.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <inttypes.h>
#include <bigendian.h>

#define TRY_ALTERNATE_HEADERS(FUNC, PARAM, SEGMENT, FIRST) \
if (altHeadersArray) { \
	const uint8_t *headers = altHeadersArray; \
	const uint8_t *dataEnd = file->dataEnd; \
	dataEnd -= 4; \
	while (headers < dataEnd) { \
		uint32_t w = u32r(headers); \
		if (w == 0 && (headers += 4)) { \
			typeof(*result) blank = { .isBlank = true }; \
			sb_push(PARAM->headers, blank); \
			continue; \
		} \
		if ((w >> 24) != SEGMENT) break; \
		if ((w & 0x00ffffff) >= file->size - 8) break; \
		if (w & 0x7) break; \
		if (data8[w & 0x00ffffff] != FIRST) break; \
		FUNC(PARAM, w); \
		headers += 4; \
	} \
}

// trim alternate headers off the end
#define TRIM_ALTERNATE_HEADERS(PARAM) \
	if (PARAM && sb_count(PARAM) > 1) \
		sb_foreach_backwards(PARAM, { \
			if (each->isBlank) \
				sb_pop(PARAM); \
			else \
				break; \
		})

// pad blank alternate headers onto the end
#define PAD_ALTERNATE_HEADERS(PARAM, HOWMANY) \
	if (PARAM && sb_count(PARAM) > 1) \
		while (sb_count(PARAM) < HOWMANY) { \
			typeof(*PARAM) blank = { .isBlank = true }; \
			sb_push(PARAM, blank); \
		}

#if 1 /* region: private function declarations */

static struct Scene *private_SceneParseAfterLoad(struct Scene *scene);
static void private_SceneParseAddHeader(struct Scene *scene, uint32_t addr);
static struct Room *private_RoomParseAfterLoad(struct Room *room);
static void private_RoomParseAddHeader(struct Room *room, uint32_t addr);
static struct Instance private_InstanceParse(const void *data);

static int FileSetError(const char *fmt, ...);

#endif /* private function declarations */

#if 1 // region: private data

static char sFileError[2048];

#endif // endregion

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
	if (!str)
		return 0;
	
	char *result = Calloc(1, strlen(str) + padding + 1);
	
	strcpy(result, str);
	
	return result;
}

char *Strdup(const char *str)
{
	if (!str)
		return 0;
	
	char *result = Calloc(1, strlen(str) + 1);
	
	strcpy(result, str);
	
	return result;
}

char *StrToLower(char *str)
{
	if (!str)
		return 0;
	
	for (char *c = str; *c; ++c)
		*c = tolower(*c);
	
	return str;
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

// Find the first occurrence of the byte string s in byte string l.
// https://opensource.apple.com/source/Libc/Libc-825.25/string/FreeBSD/memmem.c.auto.html
void *Memmem(const void *haystack, size_t haystackLen, const void *needle, size_t needleLen)
{
	register char *cur, *last;
	const char *cl = (const char *)haystack;
	const char *cs = (const char *)needle;

	/* we need something to compare */
	if (haystackLen == 0 || needleLen == 0)
		return NULL;

	/* "needle" must be smaller or equal to "haystack" */
	if (haystackLen < needleLen)
		return NULL;

	/* special case where needleLen == 1 */
	if (needleLen == 1)
		return memchr(haystack, (int)*cs, haystackLen);

	/* the last position where its possible to find "needle" in "haystack" */
	last = (char *)cl + haystackLen - needleLen;

	for (cur = (char *)cl; cur <= last; cur++)
		if (cur[0] == cs[0] && memcmp(cur, cs, needleLen) == 0)
			return cur;

	return NULL;
}

float f32r(const void *data)
{
	const uint8_t *d = data;
	
	// TODO do it without pointer casting
	uint32_t tmp = u32r(d);
	return *((float*)&tmp);
}

uint32_t f32tou32(float v)
{
	// TODO do it without pointer casting
	return *((uint32_t*)&v);
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
		basePath[count] = '\0'; // thanks valgrind -- readlink() does not null-terminate
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

struct File *FileNew(const char *filename, size_t size)
{
	struct File *result = Calloc(1, sizeof(*result));
	
	result->size = size;
	result->filename = Strdup(filename);
	if (result->filename)
		result->shortname = Strdup(MAX3(
			strrchr(result->filename, '/') + 1
			, strrchr(result->filename, '\\') + 1
			, result->filename
		));
	result->data = Calloc(1, size);
	result->dataEnd = ((uint8_t*)result->data) + result->size;
	
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
	result->data = Calloc(1, result->size + 1); // zero terminator on strings
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

const char *FileGetError(void)
{
	return sFileError;
}

int FileToFilename(struct File *file, const char *filename)
{
	FILE *fp;
	
	if (!filename || !*filename) return FileSetError("empty filename");
	if (!file) return FileSetError("empty error");
	if (!(fp = fopen(filename, "wb")))
		return FileSetError("failed to open '%s' for writing", filename);
	if (fwrite(file->data, 1, file->size, fp) != file->size)
		return FileSetError("failed to write full contents of '%s'", filename);
	if (fclose(fp))
		return FileSetError("error closing file '%s' after writing", filename);
	
	return EXIT_SUCCESS;
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
	static uint32_t eofRef = 0; // used so eof blobs have one ref each
	
	DataBlobSegmentSetup(2, scene->file->data, scene->file->dataEnd, scene->blobs);
	
	sb_foreach(scene->rooms, {
		
		DataBlobSegmentSetup(3, each->file->data, each->file->dataEnd, each->blobs);
		
		typeof(each->headers) headers = each->headers;
		
		sb_foreach(headers, {
			typeof(each->displayLists) dls = each->displayLists;
			
			sb_foreach(dls, {
				if (each->opa)
					DataBlobSegmentsPopulateFromMeshNew(each->opa, &each->opaBEU32);
				if (each->xlu)
					DataBlobSegmentsPopulateFromMeshNew(each->xlu, &each->xluBEU32);
			});
		});
		
		each->blobs = DataBlobSegmentGetHead(3);
		
		// add eof marker
		each->blobs = DataBlobPush(each->blobs, each->file->dataEnd, 0, -1, DATA_BLOB_TYPE_EOF, &eofRef);
		
		fprintf(stderr, "'%s' data blobs:\n", each->file->filename);
		DataBlobPrintAll(each->blobs);
	});
	
	scene->blobs = DataBlobSegmentGetHead(2);
	
	// add eof marker
	scene->blobs = DataBlobPush(scene->blobs, scene->file->dataEnd, 0, -1, DATA_BLOB_TYPE_EOF, &eofRef);
	
	fprintf(stderr, "'%s' data blobs:\n", scene->file->filename);
	DataBlobPrintAll(scene->blobs);
	
	// trim blobs that overlap (for smaller-than-predicted palette/texture data)
	{
		sb_array(struct DataBlob*, array) = 0;
		
		// populate
		sb_foreach(scene->rooms, {
			struct DataBlob *blobs = each->blobs;
			datablob_foreach(blobs, {
				sb_push(array, each);
			});
		});
		datablob_foreach(scene->blobs, {
			sb_push(array, each);
		});
		
		// sort using bubblesort
		{
			bool swapped;
			int n = sb_count(array);
			
			for (int i = 0; i < n - 1; ++i) {
				swapped = false;
				for (int k = 0; k < n - i - 1; k++) {
					struct DataBlob *a = array[k];
					struct DataBlob *b = array[k + 1];
					if (a->refData > b->refData) {
						array[k] = b;
						array[k + 1] = a;
						swapped = true;
					}
				}
				if (swapped == false)
					break;
			}
		}
		
		// print
		//sb_foreach(array, { fprintf(stderr, "%p\n", (*each)->refData); });
		
		// trim
		for (int i = 0; i < sb_count(array) - 1; ++i)
		{
			struct DataBlob *a = array[i];
			struct DataBlob *b = array[i + 1];
			
			if (((uint8_t*)a->refData) + a->sizeBytes > ((uint8_t*)b->refData))
			{
				a->sizeBytes =
					((uintptr_t)(b->refData))
					- ((uintptr_t)(a->refData))
				;
				
				fprintf(stderr, "trimmed blob %08x size to %08x\n"
					, a->originalSegmentAddress, a->sizeBytes
				);
			}
		}
		
		// assert data blob sizes
		for (int i = 0; i < sb_count(array) - 1; ++i)
		{
			struct DataBlob *blob = array[i];
			
			if (blob->sizeBytes > 0xffffff)
				Die("data blob type %d %08x bad sizeBytes = %08x"
					, blob->type
					, blob->originalSegmentAddress
					, blob->sizeBytes
				);
		}
		
		sb_free(array);
	}
	
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
	
	// trim blank headers off the end
	sb_foreach(scene->rooms, {
		typeof(each->headers) headers = each->headers;
		TRIM_ALTERNATE_HEADERS(headers);
	})
	TRIM_ALTERNATE_HEADERS(scene->headers);
	
	// if there are alternate headers, guarantee at least 4, as OoT requires it
	// (moving this logic into the scene writer, b/c user could still delete these)
	/*
	int minHeaders = 1 + 3;
	sb_foreach(scene->rooms, {
		typeof(each->headers) headers = each->headers;
		PAD_ALTERNATE_HEADERS(headers, minHeaders);
		each->headers = headers; // in case it got reallocated
	})
	PAD_ALTERNATE_HEADERS(scene->headers, minHeaders);
	*/
	
	fprintf(stderr, "header counts\n - %s: %d headers\n", scene->file->shortname, sb_count(scene->headers));
	sb_foreach(scene->rooms, {
		fprintf(stderr, " - %s: %d headers\n", each->file->shortname, sb_count(each->headers));
	});
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
			
			DataBlobSegmentsPopulateFromMeshNew(dlist, (void*)(limb + 8));
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

CollisionHeader *CollisionHeaderNewFromSegment(uint32_t segAddr)
{
	const uint8_t *data = n64_segment_get(segAddr);
	const uint8_t *nest;
	
	if (!data) return 0;
	
	CollisionHeader *result = Calloc(1, sizeof(*result));
	
	result->originalSegmentAddress = segAddr;
	
	result->minBounds = (Vec3s){ u16r(data + 0), u16r(data + 2), u16r(data +  4) };
	result->maxBounds = (Vec3s){ u16r(data + 6), u16r(data + 8), u16r(data + 10) };
	result->numVertices = u16r(data + 12);
	if ((nest = n64_segment_get(u32r(data + 16))))
	{
		result->vtxList = Calloc(result->numVertices, sizeof(*(result->vtxList)));
		
		for (int i = 0; i < result->numVertices; ++i)
		{
			const uint8_t *elem = nest + i * 6; // 6 bytes per entry
			
			result->vtxList[i] = (Vec3s) {
				u16r(elem + 0), u16r(elem + 2), u16r(elem + 4)
			};
		}
	}
	result->numPolygons = u16r(data + 20);
	result->numSurfaceTypes = 0;
	if ((nest = n64_segment_get(u32r(data + 24))))
	{
		result->polyList = Calloc(result->numPolygons, sizeof(*(result->polyList)));
		
		for (int i = 0; i < result->numPolygons; ++i)
		{
			const uint8_t *elem = nest + i * 16; // 16 bytes per entry

			CollisionPoly poly = {
				.type = u16r(elem + 0),
				.vtxData = { u16r(elem + 2), u16r(elem + 4), u16r(elem + 6) },
				.normal = { u16r(elem + 8), u16r(elem + 10), u16r(elem + 12) },
				.dist = u16r(elem + 14)
			};
			
			result->polyList[i] = poly;
			
			if (poly.type >= result->numSurfaceTypes)
				result->numSurfaceTypes = poly.type + 1;
		}
	}
	if ((nest = n64_segment_get(u32r(data + 28))))
	{
		result->surfaceTypeList = Calloc(result->numSurfaceTypes, sizeof(*(result->surfaceTypeList)));
		
		for (int i = 0; i < result->numSurfaceTypes; ++i)
		{
			const uint8_t *elem = nest + i * 8; // 8 bytes per entry
			uint32_t w0 = u32r(elem + 0);
			uint32_t w1 = u32r(elem + 4);
			
			result->surfaceTypeList[i] = (SurfaceType) { w0, w1 };
			
			int exitIndex = (w0 & 0x1f00) >> 8;
			int cameraIndex = w0 & 0xff;
			
			// these are 1-indexed
			if (exitIndex > result->numExits)
				result->numExits = exitIndex;
			
			if (cameraIndex >= result->numCameras)
				result->numCameras = cameraIndex + 1;
		}
	}
	if ((nest = n64_segment_get(u32r(data + 32))))
	{
		result->bgCamList = Calloc(result->numCameras, sizeof(*(result->bgCamList)));
		
		for (int i = 0; i < result->numCameras; ++i)
		{
			const uint8_t *elem = nest + i * 8; // 8 bytes per entry
			
			BgCamInfo cam = {
				.setting = u16r(elem + 0),
				.count = u16r(elem + 2),
				.dataAddr = u32r(elem + 4)
			};
			
			elem = n64_segment_get(cam.dataAddr); // bgCamFuncData
			if (!(cam.dataAddrResolved = elem))
			{
				result->bgCamList[i] = cam;
				continue;
				//Die("bgCamFuncData empty, at %08x in file", (int)(elem - data));
			}
			
			cam.data = Calloc(cam.count, sizeof(*cam.data));
			Vec3s *dataV3 = (void*)cam.data;
			for (int k = 0; k < cam.count; ++k, ++dataV3)
			{
				const uint8_t *b = elem + 6 * k; // 6 bytes per entry
				
				*dataV3 = (Vec3s) { u16r(b + 0), u16r(b + 2), u16r(b + 4) };
			}
			
			/*
			// crawlspace uses a list of points
			if (cam.setting == 0x1e)
			{
				Vec3s *pos = (void*)(&cam.bgCamFuncData);
				
				for (int k = 0; k < cam.count; ++k)
				{
					const uint8_t *b = elem + 6 * k; // 6 bytes per entry
					
					pos[k] = (Vec3s) { u16r(b + 0), u16r(b + 2), u16r(b + 4) };
				}
				
				fprintf(stderr, "crawlspace uses setting %d\n", cam.setting);
			}
			else
			{
				cam.bgCamFuncData = (BgCamFuncData) {
					.pos = { u16r(elem + 0), u16r(elem + 2), u16r(elem +  4) },
					.rot = { u16r(elem + 6), u16r(elem + 8), u16r(elem + 10) },
					.fov = u16r(elem + 12),
					.flags = u16r(elem + 14),
					.unused = u16r(elem + 16),
				};
			}
			*/
			
			result->bgCamList[i] = cam;
		}
	}
	result->numWaterBoxes = u16r(data + 36);
	if ((nest = n64_segment_get(u32r(data + 40))))
	{
		result->waterBoxes = Calloc(result->numWaterBoxes, sizeof(*(result->waterBoxes)));
		
		for (int i = 0; i < result->numWaterBoxes; ++i)
		{
			const uint8_t *elem = nest + i * 16; // 16 bytes per entry
			
			result->waterBoxes[i] = (WaterBox) {
				.xMin = u16r(elem + 0),
				.ySurface = u16r(elem + 2),
				.zMin = u16r(elem + 4),
				.xLength = u16r(elem + 6),
				.zLength = u16r(elem + 8),
				.unused = u16r(elem + 10),
				.properties = u32r(elem + 12)
			};
		}
	}
	
	return result;
}


#if 1 /* region: private function implementations */

static int FileSetError(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	
	vsnprintf(sFileError, sizeof(sFileError), fmt, args);
	
	va_end(args);
	
	return EXIT_FAILURE;
}

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
		free(result);//SceneAddHeader(scene, result);
		return;
	}
	
	// walk the header
	for (walk += addr & 0x00ffffff; (void*)walk < file->dataEnd; walk += 8)
	{
		// not a header
		if ((addr & 0xffffff) && *walk > 0x1f)
		{
			free(result);
			return;
		}
		
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
			
			case 0x03: { // collision header
				uint32_t w1 = u32r(walk + 4);
				if (scene->collisions && scene->collisions->originalSegmentAddress != w1)
					Die("multiple collision headers with different segment addresses");
				scene->collisions = CollisionHeaderNewFromSegment(w1);
				break;
			}
			
			case 0x0D: { // paths
				int numPaths = walk[1];
				const uint8_t *arr = n64_segment_get(u32r(walk + 4));
				
				for (int i = 0; i < numPaths || !numPaths; ++i, arr += 8)
				{
					const uint8_t *elem;
					int numPoints = arr[0];
					uint32_t pathStart = u32r(arr + 4);
					struct ActorPath path = { 0 };
					
					if (numPoints == 0
						|| (u32r(arr) & 0x00ffffff)
						|| (data8 + (pathStart & 0x00ffffff)) >= (uint8_t*)file->dataEnd
						|| !(elem = n64_segment_get(pathStart))
					)
						break;
					
					for (int k = 0; k < numPoints; ++k, elem += 6)
					{
						Vec3f tmp = { u16r(elem + 0), u16r(elem + 2), u16r(elem + 4) };
						
						sb_push(path.points, tmp);
					}
					
					sb_push(result->paths, path);
				}
				
				fprintf(stderr, "%08x has %d paths : {", u32r(walk + 4), sb_count(result->paths));
				sb_foreach(result->paths, { fprintf(stderr, " %d,", sb_count(each->points)); } );
				fprintf(stderr, " }\n");
				break;
			}
			
			case 0x0E: { // doorways
				int numPaths = walk[1];
				const uint8_t *arr = n64_segment_get(u32r(walk + 4));
				
				for (int i = 0; i < numPaths; ++i, arr += 16)
				{
					struct Doorway doorway = {
						.frontRoom = arr[0],
						.frontCamera = arr[1],
						.backRoom = arr[2],
						.backCamera = arr[3],
						.id = u16r(arr + 4),
						.x = u16r(arr + 6),
						.y = u16r(arr + 8),
						.z = u16r(arr + 10),
						.rot = u16r(arr + 12),
						.params = u16r(arr + 14),
					};
					
					doorway.pos = (Vec3f) { UNFOLD_VEC3(doorway) };
					
					sb_push(result->doorways, doorway);
				}
				break;
			}
			
			case 0x17: { // cutscenes
				uint32_t w1 = u32r(walk + 4);
				fprintf(stderr, "cutscene header %08x\n", w1);
				result->cutsceneOot = CutsceneOotNewFromData(n64_segment_get(w1));
				break;
			}
			
			case 0x0C: // TODO unused light settings
				break;
			
			case 0x13: { // exit list
				uint32_t w1 = u32r(walk + 4);
				if (scene->exits && scene->exitsSegAddr != w1)
				{
					const uint8_t *ref = n64_segment_get(w1);
					bool isEqual = true;
					
					sb_foreach(scene->exits, {
						if (*each != u16r(ref + eachIndex * 2))
							isEqual = false;
					});
					
					// TODO optional alternate exist list for each scene header?
					if (isEqual == false)
					{
						fprintf(stderr, "unique exit list\n");
						sb_foreach(scene->exits, {
							fprintf(stderr, " [%d] %04x vs %04x\n"
								, eachIndex, *each, u16r(ref + eachIndex * 2)
							);
						});
						//Die("multiple non-matching exit lists at different segment addresses");
					}
				}
				else
				{
					scene->exitsSegAddr = w1;
				}
				break;
			}
			
			// MM only
			case 0x02: // TODO cameras used by 1B
			case 0x1B: // TODO cameras and cutscenes used by actors
			case 0x1C: // TODO mini maps
			case 0x1E: // TODO treasure chest positions on mini-map
			case 0x1D: // unused
				break;
			
			default:
				sb_push(result->unhandledCommands, u32r(walk));
				sb_push(result->unhandledCommands, u32r(walk + 4));
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
	
	// parse exit list after header loop, b/c count is derived from collision data
	if (scene->exitsSegAddr
		&& !scene->exits
		&& scene->collisions
		&& scene->collisions->numExits
	)
	{
		const uint8_t *exitData = n64_segment_get(scene->exitsSegAddr);
		
		for (int i = 0; i < scene->collisions->numExits; ++i)
			sb_push(scene->exits, u16r(exitData + i * 2));
		
		fprintf(stderr, "exits:\n");
		sb_foreach(scene->exits, {
			fprintf(stderr, " -> %04x\n", *each);
		});
	}
	
	// add after parsing
	SceneAddHeader(scene, result);
	
	// handle alternate headers
	TRY_ALTERNATE_HEADERS(private_SceneParseAddHeader, scene, 0x02, 0x15)
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
		free(result);//RoomAddHeader(room, result);
		return;
	}
	
	// walk the header
	for (walk += addr & 0x00ffffff; (void*)walk < file->dataEnd; walk += 8)
	{
		// not a header
		if ((addr & 0xffffff) && *walk > 0x1f)
		{
			free(result);
			return;
		}
		
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
				
				result->meshFormat = d[0];
				
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
				// TODO prerender format, only grabs DL's (for viewing) and nothing else for now
				else if (d[0] == 1)
				{
					uint8_t *arr = data8 + (u32r(d + 4) & 0x00ffffff);
					
					for (int i = 0; ; ++i)
					{
						uint8_t *bin = arr + 4 * i;
						uint32_t dl = u32r(bin + 0);
						
						if (!dl)
							break;
						
						struct RoomMeshSimple tmp = {
							.opa = dl
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
			
			case 0x0C: // TODO unused light settings
				break;
			
			default:
				sb_push(result->unhandledCommands, u32r(walk));
				sb_push(result->unhandledCommands, u32r(walk + 4));
				break;
		}
	}
	
	// add after parsing
	RoomAddHeader(room, result);
	
	// handle alternate headers
	TRY_ALTERNATE_HEADERS(private_RoomParseAddHeader, room, 0x03, 0x16)
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

