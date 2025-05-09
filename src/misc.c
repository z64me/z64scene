//
// misc.h
//
// misc things
//

#include "misc.h"
#include "stretchy_buffer.h"
#include "cutscene.h"
#include "logging.h"
#include "gui.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <inttypes.h>
#include <bigendian.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

// default settings
struct ProgramIni gIni = {
	.style = {
		.theme = STYLE_THEME_LIGHT,
	},
};

#define WHERE_SETTINGS ExePath("settings.tsv")

#define INI_ARRAY_SEPARATOR "?"
#define INI_PREFIX 5 // strlen("gIni.")
#define INI_READ_INT(X) if (!strcmp((#X) + INI_PREFIX, each)) { X = atoi(next); }
#define INI_READ_STRING(X) if (!strcmp((#X) + INI_PREFIX, each)) { strcpy(X, next); }
#define INI_READ_SB_ARRAY_STRINGS(X) \
	if (!strcmp((#X) + INI_PREFIX, each)) { \
		char *work = next; \
		while (true) { \
			int len = strcspn(work, INI_ARRAY_SEPARATOR); \
			bool isLast = len == strlen(work); \
			if (isLast == false) \
				work[len] = '\0'; \
			sb_push(X, strdup(work)); \
			if (isLast == false) \
				work[len] = INI_ARRAY_SEPARATOR[0]; \
			else \
				break; \
			work += len + 1; \
		} \
	}
#define INI_WRITE_INT(X) fprintf(fp, "%s\t%d\n", (#X) + INI_PREFIX, X);
#define INI_WRITE_STRING(X) if (X && strlen(X)) fprintf(fp, "%s\t%s\n", (#X) + INI_PREFIX, X);
#define INI_WRITE_SB_ARRAY_STRINGS(X) if (X && sb_count(X)) { \
	fprintf(fp, "%s\t", (#X) + INI_PREFIX); \
	sb_foreach(X, { \
		fprintf(fp, "%s", *each); \
		if (each != &sb_last(X)) \
			fprintf(fp, INI_ARRAY_SEPARATOR); \
	}) \
	fprintf(fp, "\n"); \
}
#define INI_SETTINGS(ACTION) { \
	/* style */ \
	INI_##ACTION##_INT(gIni.style.theme) \
	\
	/* paths */ \
	INI_##ACTION##_STRING(gIni.path.mips64) \
	INI_##ACTION##_STRING(gIni.path.emulator) \
	\
	/* recents */ \
	INI_##ACTION##_SB_ARRAY_STRINGS(gIni.recent.files) \
	INI_##ACTION##_SB_ARRAY_STRINGS(gIni.recent.projects) \
	INI_##ACTION##_SB_ARRAY_STRINGS(gIni.recent.roms) \
	\
	/* previous session */ \
	INI_##ACTION##_INT(gIni.previousSession.exitedUnexpectedly) \
	INI_##ACTION##_SB_ARRAY_STRINGS(gIni.previousSession.sceneRoomFilenames) \
	\
	\
}

void WindowLoadSettings(void)
{
	// load settings from file
	if (FileExists(WHERE_SETTINGS))
	{
		struct File *file = FileFromFilename(WHERE_SETTINGS);
		STRTOK_LOOP((char*)file->data, "\r\n\t")
			INI_SETTINGS(READ)
		FileFree(file);
	}
}
void WindowSaveSettings(void)
{
	FILE *fp = fopen(WHERE_SETTINGS, "w");
	INI_SETTINGS(WRITE)
	fclose(fp);
}

static int gInstanceHandlerMm = false; // keeping as int b/c can == -1

#define TRY_ALTERNATE_HEADERS(FUNC, PARAM, SEGMENT, FIRST) \
if (altHeadersArray) { \
	const uint8_t *headers = altHeadersArray; \
	const uint8_t *dataEnd = file->dataEnd; \
	if (altHeadersCount) { \
		dataEnd = headers + altHeadersCount * 4; \
		while (headers < dataEnd) { \
			uint32_t w = u32r(headers); \
			if (w == 0) { \
				typeof(*result) blank = { .isBlank = true }; \
				sb_push(PARAM->headers, blank); \
			} \
			FUNC(PARAM, w); \
			headers += 4; \
		} \
	} \
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
// XXX leaving above code for reference, but it's better to
//     be safe than sorry (is possible to have blank headers
//     at the end of the header list, after all, so cap it at
//     a sane maximum value instead using below code)
//     i test >= b/c header[0] is not an alternate header, and
//     b/c the number of alternate headers = totalHeaders - 1
#undef TRIM_ALTERNATE_HEADERS
#define TRIM_ALTERNATE_HEADERS(PARAM) \
	if (PARAM) \
		while (sb_count(PARAM) >= MAX_SCENE_ROOM_HEADERS) { \
			if (sb_last(PARAM).isBlank) \
				sb_pop(PARAM); \
			else \
				break; \
		}

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
static struct Instance private_InstanceParse(const void *data, enum InstanceTab tab);
static void ScenePostsortSquashCameras(void *udata, uint32_t blobSizeBytes);
static void ScenePostsortSquashExits(void *udata, uint32_t blobSizeBytes);
static void ScenePostsortSquashHeaders(void *udata, uint32_t blobSizeBytes);

#endif /* private function declarations */

#if 1 // region: private data


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

const char *QuickFmt(const char *fmt, ...)
{
	static char buf[256];
	
	va_list args;
	va_start(args, fmt);
	
	vsnprintf(buf, sizeof(buf), fmt, args);
	
	va_end(args);
	
	return buf;
}

void *Calloc(size_t howMany, size_t sizeEach)
{
	void *result = calloc(howMany, sizeEach);
	
	if (!result)
		Die("memory error on Calloc(%"PRIuPTR", %"PRIuPTR")", howMany, sizeEach);
	
	return result;
}

char *StrdupPad(const char *str, int padding)
{
	if (!str)
		return 0;
	
	char *result = Calloc(1, strlen(str) + ABS(padding) + 1);
	
	// negative padding = before data
	if (padding < 0)
		result -= padding;
	
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

void *Memdup(const void *data, size_t size)
{
	void *result = malloc(size);
	
	if (result)
		memcpy(result, data, size);
	
	return result;
}

// use the Swap(a, b) macro
void (Swap)(void *a, void *b, size_t aSize, size_t bSize)
{
	static void *work = 0;
	static size_t workSize = 1024; // 1 KiB is a generous start
	size_t size = aSize;
	
	assert(aSize == bSize);
	assert(size != 0);
	
	if (size > workSize)
	{
		workSize = size * 2;
		work = realloc(work, workSize);
	}
	else if (!work)
		work = malloc(workSize);
	
	//LogDebug("a b = %p %p, size = %d", a, b, (uint32_t)size);
	memcpy(work, a, size);
	memcpy(a, b, size);
	memcpy(b, work, size);
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
void *MemmemAligned(const void *haystack, size_t haystackLen, const void *needle, size_t needleLen, size_t byteAlignment)
{
	register char *cur, *last;
	const char *cl = (const char *)haystack;
	const char *cs = (const char *)needle;

	if (!byteAlignment)
		byteAlignment = 1;

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

	for (cur = (char *)cl; cur <= last; cur += byteAlignment)
		if (cur[0] == cs[0] && memcmp(cur, cs, needleLen) == 0)
			return cur;

	return NULL;
}
void *Memmem(const void *haystack, size_t haystackLen, const void *needle, size_t needleLen)
{
	return MemmemAligned(haystack, haystackLen, needle, needleLen, 1);
}

static struct Scene *sParsingScene = 0;
static struct Room *sParsingRoom = 0;
#define ParseSegmentAddressNoPush n64_segment_get // XXX for testing, don't use this macro in production
sb_array(struct DataBlobPending, *GetPendingSegmentAddressList)(uint32_t segAddr);
void *ParseSegmentAddress(uint32_t segAddr)
{
	sb_array(struct DataBlobPending, *blobsPending) = GetPendingSegmentAddressList(segAddr);
	
	if (blobsPending)
	{
		// omit duplicates, just to be safe
		bool contains = false;
		sb_foreach(*blobsPending, {
			if (each->segAddr == segAddr) {
				contains = true;
				break;
			}
		})
		if (contains == false)
			sb_push(*blobsPending, (struct DataBlobPending) { .segAddr = segAddr });
	}
	
	return n64_segment_get(segAddr);
}
void HookSegmentAddressPostsort(uint32_t segAddr, void *udata, DataBlobCallbackFunc callback)
{
	sb_array(struct DataBlobPending, *blobsPending) = 0;
	
	blobsPending = GetPendingSegmentAddressList(segAddr);
	
	if (blobsPending)
	{
		sb_foreach(*blobsPending, {
			if (each->segAddr == segAddr) {
				sb_push(each->postsort, ((struct DataBlobCallback) {
					.func = callback,
					.udata = udata,
				}));
				return;
			}
		})
	}
}
sb_array(struct DataBlobPending, *GetPendingSegmentAddressList) (uint32_t segAddr)
{
	switch (segAddr >> 24)
	{
		case 0x02:
			return &sParsingScene->blobsPending;
			break;
		
		case 0x03:
			return &sParsingRoom->blobsPending;
			break;
	}
	
	return 0;
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

int ArrayGetIndexofMaxInt(int *array, int arrayLength)
{
	int maxIndex = 0;
	
	if (!array || arrayLength <= 0)
		return -1;
	
	for (int i = 1; i < arrayLength; ++i)
		if (array[i] > array[maxIndex])
			maxIndex = i;
	
	return maxIndex;
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
	#define TIMES 8
	static char *basePath = 0;
	static char *appendTo;
	static char *buf[TIMES];
	
	if (!basePath)
	{
	#ifdef _WIN32
		basePath = malloc(MAX_PATH);
		for (int i = 0; i < TIMES; ++i) buf[i] = malloc(MAX_PATH);
		GetModuleFileName(NULL, basePath, MAX_PATH);
	#elif __linux__
		basePath = malloc(PATH_MAX);
		for (int i = 0; i < TIMES; ++i) buf[i] = malloc(PATH_MAX);
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
		LogDebug("ExePath = '%s'", basePath);
		
		// make tmp directory while we're here
		mkdir(
			ExePath(WHERE_TMP)
			#ifndef _WIN32
			, 0777
			#endif
		);
	}
	else
		strcpy(appendTo, path);
	
	// circular buffer logic
	static int x = 0;
	char *which = buf[x];
	x += 1;
	x %= TIMES;
	strcpy(which, basePath);
	return which;
	#undef TIMES
}

struct Scene *SceneFromFilename(const char *filename)
{
	struct Scene *result = Calloc(1, sizeof(*result));
	
	result->file = FileFromFilename(filename);
	
	return private_SceneParseAfterLoad(result);
}

struct Scene *SceneFromFilenamePredictRooms(const char *filename)
{
	char *roomNameBuf = StrdupPad(filename, 100);
	struct Scene *scene = SceneFromFilename(filename);
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
			
			LogDebug("%s", roomNameBuf);
			
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
	
	SceneReady(scene);
	
	free(roomNameBuf);
	return scene;
}

struct Room *RoomFromRomOffset(struct File *rom, uint32_t romStart, uint32_t romEnd)
{
	struct Room *room = Calloc(1, sizeof(*room));
	
	room->file = FileFromData(
		((uint8_t*)rom->data) + romStart
		, romEnd - romStart
		, false
	);
	
	return private_RoomParseAfterLoad(room);
}

struct Scene *SceneFromRomOffset(struct File *rom, uint32_t romStart, uint32_t romEnd)
{
	struct Scene *scene = Calloc(1, sizeof(*scene));
	
	scene->file = FileFromData(
		((uint8_t*)rom->data) + romStart
		, romEnd - romStart
		, false
	);
	
	private_SceneParseAfterLoad(scene);
	struct SceneHeader *header = &scene->headers[0];
	for (int i = 0; i < header->numRooms; ++i)
	{
		uint32_t start = u32r(header->roomStartEndAddrs + i * 8);
		uint32_t end = u32r(header->roomStartEndAddrs + i * 8 + 4);
		
		SceneAddRoom(scene, RoomFromRomOffset(rom, start, end));
	}
	
	SceneReady(scene);
	
	return scene;
}

void SceneAddHeader(struct Scene *scene, struct SceneHeader *header)
{
	// new blank header
	if (!header)
	{
		struct SceneHeader h = {
			.scene = scene,
		};
		
		sb_push(scene->headers, h);
		
		sb_foreach(scene->rooms, {
			RoomAddHeader(each, 0);
		})
		return;
	}
	
	header->scene = scene;
	
	sb_push(scene->headers, *header);
	
	free(header);
}

#define FOR_EXTERNAL_SEGMENTS for (int i = 0x08; i <= 0x0F; ++i)
void SceneReadyDataBlobs(struct Scene *scene)
{
	static uint32_t eofRef = 0; // used so eof blobs have one ref each
	
	FOR_EXTERNAL_SEGMENTS { DatablobFreeList(DataBlobSegmentGetHead(i)); }
	
	DataBlobSegmentSetup(2, scene->file->data, scene->file->dataEnd, scene->blobs);
	
	// allows texture data blobs from unpopulated external segments, for flipbooks
	FOR_EXTERNAL_SEGMENTS { DataBlobSegmentSetup(i, 0, 0x0, 0); }
	
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
			
			// prerenders
			if (each->meshFormat == 1)
			{
				switch (each->image.base.amountType)
				{
					case ROOM_SHAPE_IMAGE_AMOUNT_SINGLE:
						DataBlobSegmentsPopulateFromRoomShapeImage(&each->image.single.image);
						break;
					
					case ROOM_SHAPE_IMAGE_AMOUNT_MULTI: {
						typeof(each->image.multi.backgrounds) backgrounds = each->image.multi.backgrounds;
						sb_foreach(backgrounds, {
							DataBlobSegmentsPopulateFromRoomShapeImage(&each->image);
						})
						break;
					}
					
					default:
						Die("unsupported prerender amountType=%d", each->image.base.amountType);
						break;
				}
			}
		});
		
		each->blobs = DataBlobSegmentGetHead(3);
		
		// add eof marker
		each->blobs = DataBlobPush(each->blobs, each->file->dataEnd, 0, -1, DATA_BLOB_TYPE_EOF, &eofRef);
		
		// add pending blobs
		sb_foreach_named(each->blobsPending, pending_, {
			struct DataBlobPending pending = *pending_;
			uint32_t segAddr = pending.segAddr;
			each->blobs = DataBlobPush(
				each->blobs
				, ((uint8_t*)each->file->data) + (segAddr & 0x00ffffff)
				, 0
				, segAddr
				, DATA_BLOB_TYPE_GENERIC
				, 0
			);
			each->blobs->callbacks = pending;
		})
		
		LogDebug("'%s' data blobs:", each->file->filename);
		DataBlobPrintAll(each->blobs);
	});
	
	scene->blobs = DataBlobSegmentGetHead(2);
	
	// add eof marker
	scene->blobs = DataBlobPush(scene->blobs, scene->file->dataEnd, 0, -1, DATA_BLOB_TYPE_EOF, &eofRef);
	
	// add pending blobs
	sb_foreach_named(scene->blobsPending, pending_, {
		struct DataBlobPending pending = *pending_;
		uint32_t segAddr = pending.segAddr;
		scene->blobs = DataBlobPush(
			scene->blobs
			, ((uint8_t*)scene->file->data) + (segAddr & 0x00ffffff)
			, 0
			, segAddr
			, DATA_BLOB_TYPE_GENERIC
			, 0
		);
		scene->blobs->callbacks = pending;
	})
	
	// determine the dimensions of flipbook textures
	// referenced by the texture lists (the sizes of
	// each flipbook texture can be derived from the
	// material associated with the ram segment that
	// is populated with each texture list)
	sb_foreach_named(scene->headers, sceneHeader, {
		sb_foreach_named(sceneHeader->mm.sceneSetupData, material, {
			int segment = ABS_ALT(material->segment) + 7;
			struct DataBlob *match = DataBlobListFindBlobWithOriginalSegmentAddress(
				DataBlobSegmentGetHead(segment)
				, segment << 24
			);
			if (!match)
				continue;
			material->datablob = match;
			LogDebug("set material segment 0x%02x -> %p, type %d", segment, match, match->type);
			if (material->type == 5) {
				AnimatedMatTexCycleParams *params = material->params;
				//LogDebug("textureList containing %d textures", sb_count(params->textureList));
				sb_foreach(params->textureList, {
					uint32_t addr = each->addr;
					struct DataBlob *blob =
					scene->blobs = DataBlobPush(
						scene->blobs
						, ((uint8_t*)scene->file->data) + (addr & 0x00ffffff)
						, match->sizeBytes
						, addr
						, DATA_BLOB_TYPE_TEXTURE
						, &each->addrBEU32
						// IMPORTANT TODO addrBEU32 refs will break on textureList
						//                reallocs, so switch to an id system later
					);
					match->data.texture.flipbookSegment = segment;
					blob->subtype = DATA_BLOB_SUBTYPE_FLIPBOOK;
					scene->blobs->data = match->data; // texture dimensions
					blob->refDataFileEnd = scene->file->dataEnd;
					each->datablob = blob;
				})
			}
		})
	})
	
	LogDebug("'%s' data blobs:", scene->file->filename);
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
		//sb_foreach(array, { LogDebug("%p", (*each)->refData); });
		
		// raise all vertex data blocks to beginning of list, arranged by address
		// (this accounts for coalesced (overlapping) vertex data blocks)
		sb_foreach_backwards(array, {
			struct DataBlob *blob = *each;
			
			if (blob->type == DATA_BLOB_TYPE_VERTEX)
			{
				switch (blob->originalSegmentAddress >> 24)
				{
					case 0x02:
						DataBlobListTouchBlob(&scene->blobs, blob);
						break;
					
					case 0x03:
						sb_foreach_named(scene->rooms, room, {
							if (blob->refData >= room->file->data
								&& blob->refData < room->file->dataEnd
							) {
								DataBlobListTouchBlob(&room->blobs, blob);
								break;
							}
						})
						break;
				}
			}
		})
		
		// trim
		for (int i = 0; i < sb_count(array) - 1; ++i)
		{
			struct DataBlob *a = array[i];
			struct DataBlob *b = array[i + 1];
			
			// don't attempt to resize empty data blobs
			if (!a->sizeBytes)
			{
				a->callbacks.sizeBytes =
					((uintptr_t)(b->refData))
					- ((uintptr_t)(a->refData))
				;
				continue;
			}
			
			// don't trim overlapping meshes
			// (aka the G_ENDDL command could get stripped off
			// the end of a display list if the G_ENDDL command
			// itself is referenced elsewhere as a blank mesh)
			if (a->type == DATA_BLOB_TYPE_MESH
				&& b->type == DATA_BLOB_TYPE_MESH
			)
			{
				/*
				// log overlaps
				if (((uint8_t*)a->refData) + a->sizeBytes > ((uint8_t*)b->refData))
				{
					FILE *fp = fopen("bin/mesh-overlaps.txt", "a+");
					
					fprintf(fp, "%08x + %08x >= %08x (%08x) -- %s\n",
						a->originalSegmentAddress, a->sizeBytes,
						b->originalSegmentAddress, b->sizeBytes,
						scene->file->filename
					);
					
					fclose(fp);
				}
				*/
				continue;
			}
			
			if (((uint8_t*)a->refData) + a->sizeBytes > ((uint8_t*)b->refData))
			{
				uint32_t oldSize = a->sizeBytes;
				uint32_t newSize =
				a->sizeBytes =
					((uintptr_t)(b->refData))
					- ((uintptr_t)(a->refData))
				;
				
				// overlapping vertex data
				if (a->type == DATA_BLOB_TYPE_VERTEX
					&& b->type == DATA_BLOB_TYPE_VERTEX
				)
					// just in case 'b' is completely encapsulated by 'a'
					b->sizeBytes = MAX(b->sizeBytes, oldSize - newSize);
				
				LogDebug("trimmed blob %08x size to %08x"
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
		
		// post-sort callbacks
		sb_foreach(array, {
			struct DataBlob *blob = *each;
			struct DataBlobPending callback = blob->callbacks;
			sb_foreach(callback.postsort, {
				each->func(each->udata, callback.sizeBytes);
			})
		})
		
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
			
			memset(each->file->data, -1, each->file->size);
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
		
		char tmp[1024];
		struct File *file = scene->file;
		memset(file->data, -1, file->size);
		datablob_foreach(scene->blobs, { memset((void*)each->refData, 0, each->sizeBytes); })
		sprintf(tmp, "%s.test", file->filename);
		FILE *fp = fopen(tmp, "wb");
		fwrite(file->data, 1, file->size, fp);
		fclose(fp);
		
		exit(0);
	}
	
	LogDebug("total texture blobs %d", sb_count(scene->textureBlobs));
	
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
	
	LogDebug("header counts\n - %s: %d headers", scene->file->shortname, sb_count(scene->headers));
	sb_foreach(scene->rooms, {
		LogDebug(" - %s: %d headers", each->file->shortname, sb_count(each->headers));
	});
}

void SceneReady(struct Scene *scene)
{
	SceneReadyDataBlobs(scene);
	
	// determine which has the lowest headers
	{
		int lowest = MIN(INT_MAX, sb_count(scene->headers));
		sb_foreach(scene->rooms, lowest = MIN(lowest, sb_count(each->headers));)
		
		// and ensure nothing has more headers than that
		while (sb_count(scene->headers) > lowest)
		{
			SceneHeaderFree(&sb_last(scene->headers));
			sb_pop(scene->headers);
		}
		sb_foreach(scene->rooms, {
			while (sb_count(each->headers) > lowest)
			{
				RoomHeaderFree(&sb_last(each->headers));
				sb_pop(each->headers);
			}
		})
	}
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
	// new blank header
	if (!header)
	{
		struct RoomHeader h = {
			.room = room,
		};
		
		sb_push(room->headers, h);
		return;
	}
	
	header->room = room;
	
	sb_push(room->headers, *header);
	
	free(header);
}

void FileFree(struct File *file)
{
	if (!file)
		return;
	
	if (file->data && file->ownsData)
		free(file->data);
	
	if (file->filename)
		free(file->filename);
	
	if (file->shortname)
		free(file->shortname);
	
	if (file->ownsHandle)
		free(file);
}

void RoomHeaderFree(struct RoomHeader *header)
{
	sb_free(header->instances);
	sb_free(header->objects);
	sb_free(header->displayLists);
	sb_free(header->unhandledCommands);
	
	// prerendered background
	if (header->meshFormat == 1)
	{
		switch (header->image.base.amountType)
		{
			case ROOM_SHAPE_IMAGE_AMOUNT_SINGLE:
				// no memory was allocated
				break;
			
			case ROOM_SHAPE_IMAGE_AMOUNT_MULTI:
				sb_free(header->image.multi.backgrounds);
				break;
		}
	}
}

void RoomFree(struct Room *room)
{
	FileFree(room->file);
	
	sb_foreach(room->headers, {
		RoomHeaderFree(each);
	});
	sb_free(room->headers);
	
	DatablobFreeList(room->blobs);
}

void SceneHeaderFree(struct SceneHeader *header)
{
	sb_free(header->spawns);
	sb_free(header->lights);
	sb_free(header->doorways);
	sb_free(header->actorCutscenes);
	sb_free(header->unhandledCommands);
	sb_free(header->specialFiles);
	
	sb_foreach(header->actorCsCamInfo, {
		sb_free(each->actorCsCamFuncData);
	})
	sb_free(header->actorCsCamInfo);
	
	sb_foreach(header->paths, {
		sb_free(each->points);
	})
	sb_free(header->paths);
	
	CutsceneOotFree(header->cutsceneOot);
	CutsceneListMmFree(header->cutsceneListMm);
	
	AnimatedMaterialFreeList(header->mm.sceneSetupData);
	
	sb_free(header->exits);
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
	
	CollisionHeaderFree(scene->collisions);
	
	DatablobFreeList(scene->blobs);
	sb_free(scene->textureBlobs);
	
	free(scene);
}

void SceneAddRoom(struct Scene *scene, struct Room *room)
{
	room->scene = scene;
	
	sb_push(scene->rooms, *room);
	
	free(room);
}

const char *SceneMigrateVisualAndCollisionData(struct Scene *dst, struct Scene *src)
{
	// safety
	if (sb_count(dst->rooms) > sb_count(src->rooms))
		return
			"cannot migrate data from scene with fewer"
			" rooms into scene with more rooms; if this"
			" is your intent, please delete the appropriate"
			" rooms from the currently loaded scene first"
			" before trying again (Tools -> Delete Room X)"
		;
	
	// ensure flipbook data will not be lost
	datablob_foreach(dst->blobs, {
		if (each->subtype == DATA_BLOB_SUBTYPE_FLIPBOOK
			&& each->ownsRefData == false
		) {
			each->refData = Memdup(each->refData, each->sizeBytes);
			each->refDataFileEnd = ((const uint8_t*)each->refData) + each->sizeBytes;
			each->ownsRefData = true;
		}
	})
	
	// migrate persistent data blobs from old scene to new scene
	{
		struct DataBlob **prev = &dst->blobs;
		struct DataBlob *next = 0;
		for (struct DataBlob *blob = *prev; blob; blob = next) {
			next = blob->next;
			if (blob->ownsRefData) {
				// link into new list:
				blob->next = src->blobs;
				src->blobs = blob;
				// unlink from old list:
				*prev = next;
				// mark stale:
				if (blob->type == DATA_BLOB_TYPE_TEXTURE) {
					blob->data.texture.pal = 0;
					if (blob->data.texture.glTexture) {
						// TODO glDeleteTextures
						blob->data.texture.glTexture = 0;
					}
				}
			} else {
				prev = &blob->next;
			}
		}
	}
	/*
	// no longer using this method, but keeping it for reference
	datablob_foreach(dst->blobs, {
		if (each->ownsRefData) {
			struct DataBlob *copy = Memdup(each, sizeof(*each));
			copy->next = src->blobs;
			src->blobs = copy;
			// hand ownership of these to the copy:
			each->ownsRefData = false;
			each->refs = 0;
		}
	})
	*/
	
	// update flipbook blobs so they reflect the new mesh
	// and point to the new palettes (when applicable)
	sb_foreach_named(dst->headers, sceneHeader, {
		sb_foreach_named(sceneHeader->mm.sceneSetupData, material, {
			int segment = ABS_ALT(material->segment) + 7;
			struct DataBlob *match = DataBlobListFindBlobWithOriginalSegmentAddress(
				DataBlobSegmentGetHead(segment)
				, segment << 24
			);
			material->datablob = match;
			if (!match) {
				LogDebug("initialize material segment 0x%02x to 0", segment);
				continue;
			}
			LogDebug("update material segment 0x%02x -> %p, type %d", segment, match, match->type);
			//LogDebug("that segment contains:");
			//DataBlobPrintAll(DataBlobSegmentGetHead(segment));
			if (material->type == 5) {
				datablob_foreach(src->blobs, {
					if (each->subtype == DATA_BLOB_SUBTYPE_FLIPBOOK
						&& each->data.texture.flipbookSegment == segment
					) {
						match->data.texture.flipbookSegment = segment;
						each->data = match->data;
					}
				})
			}
		})
	})
	
	// ensure any rooms that were added have at least as many
	// headers as the scene currently loaded in the viewport
	while (sb_count(src->headers) < sb_count(dst->headers))
		SceneAddHeader(src, 0);
	
	// swap mesh data
	sb_foreach_named(dst->rooms, dstRoom, {
		// dst scene has more rooms than src scene
		if (dstRoomIndex >= sb_count(src->rooms))
			break;
		struct Room *srcRoom = &src->rooms[dstRoomIndex];
		for (int i = 0; i < sb_count(dstRoom->headers); ++i) {
			struct RoomHeader *a = &dstRoom->headers[i];
			struct RoomHeader *b = &srcRoom->headers[i];
			
			Swap(&a->displayLists, &b->displayLists);
			Swap(&a->meshFormat, &b->meshFormat);
			Swap(&a->image, &b->image);
		}
		Swap(&dstRoom->file, &srcRoom->file);
		Swap(&dstRoom->blobs, &srcRoom->blobs);
		if (dstRoom->file && srcRoom->file) {
			Swap(&dstRoom->file->filename, &srcRoom->file->filename);
			Swap(&dstRoom->file->shortname, &srcRoom->file->shortname);
		}
	})
	
	// migrate any rooms that were added
	int firstNewRoom = sb_count(dst->rooms);
	for (int i = firstNewRoom; i < sb_count(src->rooms); ++i) {
		struct Room *srcRoom = &src->rooms[i];
		struct File *srcFile = srcRoom->file;
		const char *first = dst->rooms[0].file->filename;
		const char *extension = strrchr(first, '.');
		if (srcFile->filename) free(srcFile->filename);
		srcFile->filename = StrdupPad(first, 10);
		sprintf(strrchr(srcFile->filename, '_'), "_%d%s", i, extension);
		sb_push(dst->rooms, *srcRoom);
		LogDebug("added room '%s' during migration", srcFile->filename);
	}
	while (sb_count(src->rooms) > firstNewRoom)
		sb_pop(src->rooms);
	sb_foreach(dst->headers, { each->numRooms = sb_count(dst->rooms); })
	
	// swap collision data and blobs
	Swap(&dst->collisions, &src->collisions);
	Swap(&dst->blobs, &src->blobs);
	Swap(&dst->textureBlobs, &src->textureBlobs);
	Swap(&dst->file, &src->file);
	Swap(&dst->file->filename, &src->file->filename);
	Swap(&dst->file->shortname, &src->file->shortname);
	
	//
	//LogDebug("dst datablobs post migration:");
	//DataBlobPrintAll(dst->blobs);
	
	// success
	return 0;
}

struct Room *RoomFromFilename(const char *filename)
{
	struct Room *result = Calloc(1, sizeof(*result));
	
	result->file = FileFromFilename(filename);
	
	return private_RoomParseAfterLoad(result);
}

void ScenePopulateRoom(struct Scene *scene, int index, struct Room *room);

CollisionHeader *CollisionHeaderNewFromSegment(uint32_t segAddr, uint32_t fileSize)
{
	const uint8_t *data = ParseSegmentAddress(segAddr);
	const uint8_t *dataStart = ParseSegmentAddress(segAddr & 0xff000000);
	const uint8_t *dataEnd = (dataStart) ? dataStart + fileSize : 0;
	const uint8_t *nest;
	
	if (!data) return 0;
	
	CollisionHeader *result = Calloc(1, sizeof(*result));
	
	result->originalSegmentAddress = segAddr;
	
	result->minBounds = (Vec3s){ u16r(data + 0), u16r(data + 2), u16r(data +  4) };
	result->maxBounds = (Vec3s){ u16r(data + 6), u16r(data + 8), u16r(data + 10) };
	result->numVertices = u16r(data + 12);
	if ((nest = ParseSegmentAddress(u32r(data + 16))))
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
	if ((nest = ParseSegmentAddress(u32r(data + 24))))
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
	if ((nest = ParseSegmentAddress(u32r(data + 28))))
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
	if ((nest = ParseSegmentAddress(u32r(data + 32))))
	{
		HookSegmentAddressPostsort(u32r(data + 32), result, ScenePostsortSquashCameras);
		
		for (int i = 0; ; ++i)
		{
			const uint8_t *elem = nest + i * 8; // 8 bytes per entry
			
			// don't read past the eof
			if (dataEnd && elem + 8 > dataEnd)
				break;
			
			BgCamInfo cam = {
				.setting = u16r(elem + 0),
				.count = u16r(elem + 2),
				.dataAddr = u32r(elem + 4)
			};
			
			// get info about all oot/mm scenes so can look for patterns
			/*
			static BgCamInfo test;
			static FILE *log;
			if (!log) log = fopen("bin/info.txt", "wb");
			if (cam.setting > test.setting) fprintf(log, "setting = 0x%04x\n", (test.setting = cam.setting));
			if (cam.count > test.count) fprintf(log, "count = 0x%04x\n", (test.count = cam.count));
			if (cam.count % 3) Die("cam.count %% 3");
			if (cam.count == 0 && cam.dataAddr) Die("cam.count == 0");
			*/
			
			// sanity check for possible cameras not referenced by collision surface types
			{
				if (cam.setting >= MAX(CAM_SET_MAX_OOT, CAM_SET_MAX_MM)
					|| (cam.dataAddr && !cam.count) // count is always non-zero if address is given
					|| (cam.count && !cam.dataAddr) // count is always 0 if no address is given
					|| (cam.count % 3) // count is always multiple of 3
					|| (cam.count > 9) // always == 3 in mm, oot uses 6 and 9 for crawlspaces
					|| (cam.dataAddr & 1) // not 2-byte aligned
					|| (cam.dataAddr && (cam.dataAddr >> 24) != (segAddr >> 24)) // should ref input segment
					|| ((cam.dataAddr & 0x00ffffff) + cam.count * 6) > fileSize // data runs past eof
				)
				{
					if (i < result->numCameras)
						LogDebug("camera list ended earlier than expected");
					break;
				}
				
				if (i >= result->numCameras)
					LogDebug("adding unreferenced camera %04x %04x %08x", cam.setting, cam.count, cam.dataAddr);
			}
			
			elem = ParseSegmentAddress(cam.dataAddr); // bgCamFuncData
			if (!(cam.dataAddrResolved = elem))
			{
				sb_push(result->bgCamList, cam);
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
				
				LogDebug("crawlspace uses setting %d", cam.setting);
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
			
			sb_push(result->bgCamList, cam);
		}
		
		// update camera count, in case any were added
		result->numCameras = sb_count(result->bgCamList);
	}
	else
	{
		// guarantee a blank camera exists, for MM compatibility
		sb_push(result->bgCamList, (BgCamInfo){0});
	}
	result->numWaterBoxes = u16r(data + 36);
	if ((nest = ParseSegmentAddress(u32r(data + 40))))
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
	
	// masked tri list
	result->triListMasked = Calloc(result->numPolygons, sizeof(*(result->triListMasked)));
	for (int i = 0; i < result->numPolygons; ++i)
	{
		uint16_t flags = 0;
		
		for (int k = 0; k < 3; ++k)
			flags |= result->polyList[i].vtxData[k];
		
		// collision with this flag is ignored by most entities
		if (flags & 0x4000)
			continue;
		
		result->triListMasked[i] = (Vec3s) {
			UNFOLD_ARRAY_3_EXT(uint16_t,
				result->polyList[i].vtxData,
				& 0x1fff
			)
		};
	}
	
	return result;
}

void CollisionHeaderFreeCamera(BgCamInfo cam)
{
	if (cam.data)
		free(cam.data);
}

void CollisionHeaderFree(CollisionHeader *header)
{
	if (!header)
		return;
	
	free(header->vtxList);
	free(header->polyList);
	free(header->surfaceTypeList);
	
	if (header->triListMasked)
		free(header->triListMasked);
	
	sb_foreach(header->bgCamList, {
		CollisionHeaderFreeCamera(*each);
	})
	sb_free(header->bgCamList);
	
	free(header->waterBoxes);
	
	free(header);
}

// not actually a uuid, but close enough
uint32_t InstanceNewUuid(void)
{
	static uint32_t s = 0;
	
	return ++s;
}

// experimented with polymorphic instance types
/*
struct Instance *InstanceAddToListGeneric(struct Instance **list, const void *src)
{
	const struct Instance *inst = src;
	
	switch (inst->tab)
	{
		case INSTANCE_TAB_ACTOR:
			sb_push(*list, *inst);
			break;
		
		case INSTANCE_TAB_DOOR: {
			const struct Doorway *inst = src;
			sb_push(*((struct Doorway**)list), *inst);
			break;
		}
		
		case INSTANCE_TAB_SPAWN: {
			const struct SpawnPoint *inst = src;
			sb_push(*((struct SpawnPoint**)list), *inst);
			break;
		}
		
		default:
			Die("InstanceAddToListGeneric() unknown tab type");
			break;
	}
	
	return &sb_last(*list);
}

void InstanceDeleteFromListGeneric(struct Instance **list, const void *src)
{
	const struct Instance *inst = src;
	int index = -1;
	
	sb_foreach(*list, {
		if (each == inst) {
			index = eachIndex;
			break;
		}
	})
	
	if (index < 0)
		Die("trying to delete item, doesn't exist");
	
	int copyHowMany = sb_count(*list) - (index + 1);
	int copySizeEach = 0;
	
	// popping off the end of the list
	if (copyHowMany <= 0)
	{
		sb_pop(*list);
		return;
	}
	
	switch (inst->tab)
	{
		case INSTANCE_TAB_ACTOR:
			copySizeEach = sizeof(struct Instance);
			break;
		
		case INSTANCE_TAB_DOOR: {
			copySizeEach = sizeof(struct Doorway);
			break;
		}
		
		case INSTANCE_TAB_SPAWN: {
			copySizeEach = sizeof(struct SpawnPoint);
			break;
		}
		
		default:
			Die("InstanceDeleteFromListGeneric() unknown tab type");
			break;
	}
	
	uint8_t *listRaw = (void*)*list;
	memmove(
		&listRaw[copySizeEach * index]
		, &listRaw[copySizeEach * (index + 1)]
		, copySizeEach * copyHowMany
	);
	
	sb_pop(*list);
}
*/

struct Instance InstanceMakeWritable(struct Instance inst)
{
	// mm stuff
	if (gInstanceHandlerMm)
	{
		inst.id &= 0xfff;
		#define HANDLE_AXIS(AXIS, MASK) \
			if (inst.mm.useDegreeRotationFor.AXIS) \
				inst.id |= MASK; \
			else \
				inst.AXIS##rot = rintf(inst.AXIS##rot / 182.04444444444444444f); \
			inst.AXIS##rot <<= 7;
		HANDLE_AXIS(y, 0x8000)
		HANDLE_AXIS(x, 0x4000)
		HANDLE_AXIS(z, 0x2000)
		inst.yrot |= inst.mm.csId & 0x7f;
		inst.xrot |= (inst.mm.halfDayBits >> 7) & 0x7f;
		inst.zrot |= inst.mm.halfDayBits & 0x7f;
		#undef HANDLE_AXIS
	}
	
	return inst;
}

struct Instance InstanceMakeReadable(struct Instance inst)
{
	// mm stuff
	if (gInstanceHandlerMm)
	{
		inst.mm.halfDayBits = ((inst.xrot & 0x7f) << 7) | (inst.zrot & 0x7f);
		inst.mm.csId = inst.yrot & 0x7f;
		// TODO compensate if useDegreeRotationFor differs from actorDatabase
		#define HANDLE_AXIS(AXIS, MASK) \
			inst.mm.useDegreeRotationFor.AXIS = inst.id & MASK; \
			inst.AXIS##rot >>= 7; \
			if (!inst.mm.useDegreeRotationFor.AXIS) \
				inst.AXIS##rot = rintf(inst.AXIS##rot * 182.04444444444444444f);
		HANDLE_AXIS(y, 0x8000)
		HANDLE_AXIS(x, 0x4000)
		HANDLE_AXIS(z, 0x2000)
		inst.id &= 0xfff;
		#undef HANDLE_AXIS
	}
	
	// oot default + safety for mm door actors
	if (!inst.mm.halfDayBits)
		inst.mm.halfDayBits = 0xffff;
	
	return inst;
}


#if 1 /* region: private function implementations */

static struct Instance private_InstanceParse(const void *data, enum InstanceTab tab)
{
	const uint8_t *data8 = data;
	
	struct Instance inst = {
		.id = u16r(data8 + 0)
		, .pos = { s16r3(data8 + 2) }
		, .prev = INSTANCE_PREV_INIT
		, .xrot = s16r(data8 + 8)
		, .yrot = s16r(data8 + 10)
		, .zrot = s16r(data8 + 12)
		, .params = u16r(data8 + 14)
		, .tab = tab
	};
	
	return InstanceMakeReadable(inst);
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
	uint8_t *dataEnd8 = file->dataEnd;
	uint8_t *walk = file->data;
	struct {
		uint8_t *positions;
		uint8_t *entrances;
		int count;
		int countEntrances; // unused for now, but 'entrances' is a LUT into 'positions', so counts can vary
		uint32_t posSegAddr;
	} spawnPoints = {0};
	uint8_t *altHeadersArray = 0;
	result->mm.sceneSetupType = -1;
	int exitsCount = 0;
	int altHeadersCount = 0;
	uint32_t altHeadersSegAddr = 0;
	uint32_t exitsSegAddr = 0;
	
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
	for (walk = ParseSegmentAddress(addr); (void*)walk < file->dataEnd; walk += 8)
	{
		uint32_t w1 = u32r(walk + 4);
		
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
				spawnPoints.positions = ParseSegmentAddress(w1);
				spawnPoints.count = walk[1]; // approximation, acceptable for now
				spawnPoints.posSegAddr = w1;
				break;
			
			case 0x06: // spawn point entrances
				spawnPoints.entrances = ParseSegmentAddress(w1);
				spawnPoints.countEntrances = walk[1];
				break;
			
			case 0x07: // field/dungeon objects
				if (result->specialFiles)
					Die("scene header command 07 used multiple times at %08x"
						, (uint32_t)(walk - data8)
					);
				
				(void)sb_add(result->specialFiles, 1);
				result->specialFiles->fairyHintsId = walk[1];
				result->specialFiles->subkeepObjectId = u16r(walk + 6);
				// doesn't reference data, so no sb_udata_segaddr
				// (may end up matching that to objectList for corresponding RoomHeader)
				break;
			
			case 0x04: // room list
				result->numRooms = walk[1];
				result->roomStartEndAddrs = ParseSegmentAddress(w1);
				break;
			
			case 0x0F: // light list
				uint8_t *arr = ParseSegmentAddress(w1);
				
				for (int i = 0; i < walk[1]; ++i)
					sb_push(result->lights
						, private_ZeldaLightParse(arr + 0x16 * i)
					);
				//sb_udata_segaddr(result->lights) = w1;
				break;
			
			case 0x18: { // alternate headers
				altHeadersArray = ParseSegmentAddress(w1);
				altHeadersCount = walk[1];
				altHeadersSegAddr = w1;
				break;
			}
			
			case 0x1A: { // mm texture animation
				result->mm.sceneSetupType = 1;
				result->mm.sceneSetupData = AnimatedMaterialNewFromSegment(w1);
				sb_foreach(result->mm.sceneSetupData, {
					LogDebug("texanim[%d] type%d, seg%d", eachIndex, each->type, each->segment);
				});
				//sb_udata_segaddr(result->mm.sceneSetupData) = w1;
				//Die("%d texanims", sb_count(result->mm.sceneSetupData));
				break;
			}
			
			case 0x03: { // collision header
				if (scene->collisions && scene->collisions->originalSegmentAddress != w1)
					Die("multiple collision headers with different segment addresses");
				else if (!scene->collisions)
					scene->collisions = CollisionHeaderNewFromSegment(w1, file->size);
				break;
			}
			
			case 0x0D: { // paths
				int numPaths = walk[1];
				const uint8_t *arr = ParseSegmentAddress(w1);
				
				for (int i = 0; i < numPaths || !numPaths; ++i, arr += 8)
				{
					const uint8_t *elem;
					int numPoints = arr[0];
					uint32_t pathStart = u32r(arr + 4);
					struct ActorPath path = {
						.additionalPathIndex = arr[1], // mm only
						.customValue = u16r(arr + 2), // mm only
					};
					
					if (numPoints == 0
						|| (data8 + (pathStart & 0x00ffffff)) >= (uint8_t*)file->dataEnd
						|| !(elem = ParseSegmentAddress(pathStart))
					)
						break;
					
					for (int k = 0; k < numPoints; ++k, elem += 6)
					{
						sb_push(path.points, ((struct Instance) {
							.pos = s16r3(elem),
							INSTANCE_DEFAULT_PATHPOINT
						}));
					}
					
					// first and last points are the same
					path.isClosed = (numPoints > 3
						&& !memcmp(&sb_last(path.points).pos
							, &path.points->pos
							, sizeof(Vec3f)
						)
					);
					
					sb_push(result->paths, path);
				}
				//sb_udata_segaddr(result->paths) = w1;
				
				LogDebug("%08x has %d paths : {", w1, sb_count(result->paths));
				sb_foreach(result->paths, { LogDebug(" -> %d points,", sb_count(each->points)); } );
				LogDebug("}");
				break;
			}
			
			case 0x0E: { // doorways
				int numPaths = walk[1];
				const uint8_t *arr = ParseSegmentAddress(w1);
				
				for (int i = 0; i < numPaths; ++i, arr += 16)
				{
					struct Instance doorway = {
						.id = u16r(arr + 4),
						.pos = { s16r3(arr + 6) },
						.prev = INSTANCE_PREV_INIT,
						.yrot = u16r(arr + 12),
						.params = u16r(arr + 14),
						.tab = INSTANCE_TAB_DOOR,
						.doorway = {
							.frontRoom = arr[0],
							.frontCamera = arr[1],
							.backRoom = arr[2],
							.backCamera = arr[3],
						}
					};
					doorway = InstanceMakeReadable(doorway);
					
					sb_push(result->doorways, doorway);
				}
				//sb_udata_segaddr(result->doorways) = w1;
				break;
			}
			
			case 0x17: { // cutscenes
				LogDebug("cutscene header %08x", w1);
				if (walk[1])
					result->cutsceneListMm = CutsceneListMmNewFromData(ParseSegmentAddress(w1), file->dataEnd, walk[1]);
				else
					result->cutsceneOot = CutsceneOotNewFromData(ParseSegmentAddress(w1), file->dataEnd);
				break;
			}
			
			case 0x0C: // TODO unused light settings
				break;
			
			case 0x13: { // exit list
				exitsSegAddr = w1;
				exitsCount = walk[1];
				break;
			}
			
			// MM only
			case 0x02: { // actor cutscene camera data
				const uint8_t *d = ParseSegmentAddress(w1);
				
				if (w1 && d)
				{
					for (int i = 0; i < walk[1]; ++i, d += 8)
					{
						ActorCsCamInfo tmp = { .setting = u16r(d) };
						const uint8_t *arr = ParseSegmentAddress(u32r(d + 4));
						
						for (int num = u16r(d + 2); num; --num, arr += 6)
						{
							sb_push(tmp.actorCsCamFuncData
								, ((Vec3s){ u16r3(arr) })
							);
						}
						
						sb_push(result->actorCsCamInfo, tmp);
					}
					//sb_udata_segaddr(result->actorCsCamInfo) = w1;
				}
				break;
			}
			case 0x1B: { // actor cutscene list
				const uint8_t *d = ParseSegmentAddress(w1);
				
				if (w1 && d)
				{
					for (int i = 0; i < walk[1]; ++i, d += 16)
					{
						sb_push(result->actorCutscenes,
							((ActorCutscene){
								.priority = u16r(d),
								.length = u16r(d + 2),
								.csCamId = u16r(d + 4),
								.scriptIndex = u16r(d + 6),
								.additionalCsId = u16r(d + 8),
								.endSfx = u8r(d + 10),
								.customValue = u8r(d + 11),
								.hudVisibility = u16r(d + 12),
								.endCam = u8r(d + 14),
								.letterboxSize = u8r(d + 15),
							})
						);
					}
					//sb_udata_segaddr(result->actorCutscenes) = w1;
				}
				break;
			}
			case 0x1C: // TODO mini maps
			case 0x1E: // TODO treasure chest positions on mini-map
			case 0x1D: // unused
				break;
			
			default:
				sb_push(result->unhandledCommands, u32r(walk));
				sb_push(result->unhandledCommands, w1);
				break;
		}
	}
	
	// consolidate spawnPoints to friendlier format for viewing and editing
	if (spawnPoints.positions && spawnPoints.entrances)
	{
		for (int i = 0; i < spawnPoints.count; ++i)
		{
			struct Instance tmp = private_InstanceParse(
				spawnPoints.positions + 16 * spawnPoints.entrances[i * 2]
				, INSTANCE_TAB_SPAWN
			);
			tmp.spawnpoint.room = spawnPoints.entrances[i * 2 + 1];
			
			sb_push(result->spawns, tmp);
		}
		//sb_udata_segaddr(result->spawns) = spawnPoints.posSegAddr;
	}
	
	// parse exit list after header loop, b/c count is derived from collision data
	if (exitsSegAddr
		&& (exitsCount
			|| (scene->collisions
				&& scene->collisions->numExits
			)
		)
	)
	{
		const uint8_t *exitData = ParseSegmentAddress(exitsSegAddr);
		
		if (!exitsCount)
		{
			exitsCount = scene->collisions->numExits;
			
			// just in case there are exits not referenced by
			// collision, such as those used by warp point actors
			// (the postsort callback will trim any bytes that were
			// read from subsequent blocks, so this is okay)
			exitsCount = SCENE_MAX_EXITS;
			sb_new_size(result->exits, SCENE_MAX_EXITS); // prevent realloc from breaking callback
			HookSegmentAddressPostsort(exitsSegAddr, result->exits, ScenePostsortSquashExits);
		}
		
		for (int i = 0; i < exitsCount && exitData + 2 <= dataEnd8; ++i, exitData += 2)
			sb_push(result->exits, u16r(exitData));
		
		LogDebug("exits:");
		sb_foreach(result->exits, {
			LogDebug(" -> %04x", *each);
		});
		
		//sb_udata_segaddr(result->exits) = exitsSegAddr;
	}
	
	// add after parsing
	SceneAddHeader(scene, result);
	
	// handle alternate headers
	TRY_ALTERNATE_HEADERS(private_SceneParseAddHeader, scene, 0x02, 0x15)
	
	HookSegmentAddressPostsort(altHeadersSegAddr, scene, ScenePostsortSquashHeaders);
}

static void ScenePostsortSquashCameras(void *udata, uint32_t blobSizeBytes)
{
	CollisionHeader *header = udata;
	int maxCapacity = blobSizeBytes / 8;
	
	while (sb_count(header->bgCamList) > maxCapacity)
		CollisionHeaderFreeCamera(sb_pop(header->bgCamList));
	
	// collecting info
	if (header->numCameras != sb_count(header->bgCamList)) {
		BgCamInfo *try = &header->bgCamList[sb_count(header->bgCamList)];
		LogDebug("resized from %d to %d, %d bytes, info = %d %d %08x\n"
			, header->numCameras, sb_count(header->bgCamList), blobSizeBytes
			, try->setting, try->count, try->dataAddr
		);
	}
	
	// update camera count, in case any were removed
	header->numCameras = sb_count(header->bgCamList);
}

static void ScenePostsortSquashExits(void *udata, uint32_t blobSizeBytes)
{
	sb_array(uint16_t, exits) = udata;
	int maxCapacity = blobSizeBytes / sizeof(uint16_t);
	int count = sb_count(exits);
	
	if (count > maxCapacity)
	{
		LogDebug("squash exit count from %d -> %d", count, maxCapacity);
		sb_trim(exits, maxCapacity);
	}
}

static void ScenePostsortSquashHeaders(void *udata, uint32_t blobSizeBytes)
{
	struct Scene *scene = udata;
	int maxCapacity = blobSizeBytes / sizeof(uint32_t);
	int count = sb_count(scene->headers);
	
	// subtracting 1 accounts for header[0], which isn't an alternate header
	count -= 1;
	
	if (count > maxCapacity)
	{
		LogDebug("squash alternate header count from %d -> %d", count, maxCapacity);
		
		while (count > maxCapacity)
		{
			LogDebug(" -> pop header %08x", sb_last(scene->headers).addr);
			SceneHeaderFree(&sb_last(scene->headers));
			sb_pop(scene->headers);
			--count;
		}
	}
}

static RoomShapeImage RoomShapeImageFromBytes(const void *data)
{
	const uint8_t *work = data;
	
	RoomShapeImage result = {
		.source = u32r(work + 0),
		.unk_0C = u32r(work + 4),
		.tlut = u32r(work + 8),
		.width = u16r(work + 12),
		.height = u16r(work + 14),
		.fmt = u8r(work + 16),
		.siz = u8r(work + 17),
		.tlutMode = u16r(work + 18),
		.tlutCount = u16r(work + 20),
	};
	
	LogDebug("RoomShapeImageFromBytes()");
	LogDebug(" - source    = %08x", result.source);
	LogDebug(" - tlut      = %08x", result.tlut);
	LogDebug(" - unk_0C    = %08x", result.unk_0C);
	LogDebug(" - unk_0C    = %08x", result.unk_0C);
	LogDebug(" - fmt       = %02x", result.fmt);
	LogDebug(" - siz       = %02x", result.siz);
	LogDebug(" - tlutMode  = %08x", result.tlutMode);
	LogDebug(" - tlutCount = %08x", result.tlutCount);
	
	return result;
}

static void private_RoomParseAddHeader(struct Room *room, uint32_t addr)
{
	struct RoomHeader *result = Calloc(1, sizeof(*result));
	struct File *file = room->file;
	uint8_t *data8 = file->data;
	uint8_t *walk = file->data;
	uint8_t *altHeadersArray = 0;
	int altHeadersCount = 0;
	
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
	for (walk = ParseSegmentAddress(addr); (void*)walk < file->dataEnd; walk += 8)
	{
		uint32_t w1 = u32r(walk + 4);
		
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
				uint8_t *arr = ParseSegmentAddress(w1);
				
				for (int i = 0; i < walk[1]; ++i)
					sb_push(result->instances
						, private_InstanceParse(arr + 16 * i, INSTANCE_TAB_ACTOR)
					);
				//sb_udata_segaddr(result->instances) = w1;
				break;
			}
			
			case 0x0B: { // object id's
				uint8_t *arr = ParseSegmentAddress(w1);
				
				for (int i = 0; i < walk[1]; ++i)
					sb_push(result->objects
						, ((struct ObjectEntry) {
							.id = u16r(arr + 2 * i),
							.type = OBJECT_ENTRY_TYPE_EXPLICIT,
						})
					);
				//sb_udata_segaddr(result->objects) = w1;
				break;
			}
			
			case 0x0A: { // mesh header
				uint8_t *d = ParseSegmentAddress(w1);
				
				result->meshFormat = d[0];
				
				if (d[0] == 2)
				{
					int num = d[1];
					uint8_t *arr = ParseSegmentAddress(u32r(d + 4));
					
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
					uint8_t *arr = ParseSegmentAddress(u32r(d + 4));
					
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
				// maps that use prerendered backgrounds
				else if (d[0] == 1)
				{
					uint8_t *arr = ParseSegmentAddress(u32r(d + 4));
					
					// DL's
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
					
					// type
					switch (d[1])
					{
						case ROOM_SHAPE_IMAGE_AMOUNT_SINGLE:
							result->image.single = (RoomShapeImageSingle){
								.image = (RoomShapeImageFromBytes(d + 8)),
							};
							break;
						
						case ROOM_SHAPE_IMAGE_AMOUNT_MULTI: {
							result->image.multi = (RoomShapeImageMulti){
								.numBackgrounds = u8r(d + 8),
							};
							LogDebug("prerender w/ %d bg's", result->image.multi.numBackgrounds);
							const uint8_t *work = ParseSegmentAddress(u32r(d + 12));
							LogDebug("work = %08x %p", (u32r(d + 12) & 0x00ffffff), work);
							for (int i = 0; i < result->image.multi.numBackgrounds; ++i, work += 28)
							{
								sb_push(result->image.multi.backgrounds
									, ((RoomShapeImageMultiBgEntry){
										.unk_00 = u16r(work + 0),
										.bgCamIndex = u8r(work + 2),
										.image = RoomShapeImageFromBytes(work + 4),
									})
								);
							}
							break;
						}
						
						default:
							Die("unknown prerender amountType=%d", d[1]);
							break;
					}
					result->image.base.amountType = d[1];
				}
				else
				{
					LogDebug("unsupported mesh header type %d", d[0]);
				}
				//sb_udata_segaddr(result->displayLists) = w1;
				break;
			}
			
			case 0x18: { // alternate headers
				altHeadersArray = ParseSegmentAddress(w1);
				altHeadersCount = walk[1];
				break;
			}
			
			case 0x0C: // TODO unused light settings
				break;
			
			default:
				sb_push(result->unhandledCommands, u32r(walk));
				sb_push(result->unhandledCommands, w1);
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
	sParsingScene = scene;
	
	// just in case user alternates between MM and OoT scenes
	gInstanceHandlerMm = false;
	for (const uint8_t *tmp = file->data; *tmp != 0x14; tmp += 8)
		if (*tmp == 0x1B)
			gInstanceHandlerMm = true;
	
	private_SceneParseAddHeader(scene, 0x02000000);
	
	ON_CHANGE_DEFAULT(gInstanceHandlerMm, -1)
	{
		switch (gInstanceHandlerMm)
		{
			case false:
				GuiLoadBaseDatabases("oot");
				break;
			
			case true:
				GuiLoadBaseDatabases("mm");
				break;
		}
	}
	
	return scene;
}

static struct Room *private_RoomParseAfterLoad(struct Room *room)
{
	assert(room);
	
	struct File *file = room->file;
	
	assert(file);
	assert(file->data);
	assert(file->size);
	
	n64_segment_set(0x03, file->data);
	sParsingRoom = room;
	
	private_RoomParseAddHeader(room, 0x03000000);
	
	return room;
}

#endif /* private functions */

