//
// scene-write.c
//
// scene/room file writer
//

#include <stdio.h>

#include "misc.h"
#include "cutscene.h"

static struct File *gWork = 0;
static struct DataBlob *gWorkblob = 0;
static uint32_t gWorkblobAddr = 0;
static uint32_t gWorkblobAddrEnd = 0;
static uint32_t gWorkblobSegment = 0;
static uint8_t *gWorkblobData = 0;
static bool gWorkblobAllowDuplicates = false;
static uint32_t gWorkblobExactlyThisSize = 0;
static uint32_t gWorkblobExactlyThisSizeStartSize = 0;
static uint32_t gWorkFindAlignment = 0;
#define WORKBUF_SIZE (1024 * 1024 * 4) // 4mib is generous
#define WORKBLOB_STACK_SIZE 32
#define FIRST_HEADER_SIZE 0x100 // sizeBytes of first header in file
#define FIRST_HEADER &(struct DataBlob){ .sizeBytes = FIRST_HEADER_SIZE }
static struct DataBlob gWorkblobStack[WORKBLOB_STACK_SIZE];
void CollisionHeaderToWorkblob(CollisionHeader *header);

// invoke once on program exit for cleanup
void SceneWriterCleanup(void)
{
	if (gWorkblobData)
	{
		free(gWorkblobData);
		gWorkblobData = 0;
	}
	
	if (gWork)
	{
		FileFree(gWork);
		gWork = 0;
	}
}

// guarantee a minimum number of alternate headers are written
#define PAD_ALTERNATE_HEADERS(PARAM, HOWMANY) \
	while (gWorkblob->sizeBytes < (HOWMANY * 4)) \
		WorkblobPut32(0);
#define MIN_ALTERNATE_HEADERS 3

static uint32_t Swap32(const uint32_t b)
{
	uint32_t result = 0;
	
	result |= ((b >> 24) & 0xff) <<  0;
	result |= ((b >> 16) & 0xff) <<  8;
	result |= ((b >>  8) & 0xff) << 16;
	result |= ((b >>  0) & 0xff) << 24;
	
	return result;
}

static void WorkReady(void)
{
	// big alignment for meshes and textures etc
	gWorkFindAlignment = 8;
	
	if (!gWork)
		gWork = FileNew("work", WORKBUF_SIZE);
	
	if (!gWorkblobData)
	{
		gWorkblobData = Calloc(1, WORKBUF_SIZE);
		gWorkblob = gWorkblobStack - 1;
	}
	
	gWork->size = 0;
}

static uint32_t WorkFindDatablob(struct DataBlob *blob)
{
	const uint8_t *needle = blob->refData;
	const uint8_t *haystack = gWork->data;
	
	if (gWorkblobAllowDuplicates)
		return 0;
	
	if (!needle || gWork->size <= FIRST_HEADER_SIZE)
		return 0;
	
	const uint8_t *match = MemmemAligned(
		haystack + FIRST_HEADER_SIZE
		, gWork->size - FIRST_HEADER_SIZE
		, needle
		, blob->sizeBytes
		, gWorkFindAlignment
	);
	
	if (match)
		return ((blob->updatedSegmentAddress =
			(blob->originalSegmentAddress & 0xff000000)
			| ((match - haystack) & 0x00ffffff)
		));
	
	return 0;
	
}

static uint32_t WorkAppendDatablob(struct DataBlob *blob)
{
	uint8_t *dest = ((uint8_t*)gWork->data) + gWork->size;
	uint32_t addr;
	
	if (!blob->sizeBytes)
		return 0;
	
	while (gWorkblob->alignBytes
		&& (gWork->size % gWorkblob->alignBytes)
	)
	{
		*dest = 0;
		
		gWork->size++;
		++dest;
	}
	
	if ((addr = WorkFindDatablob(blob)))
		return addr;
	
	if (blob->refData)
		memcpy(dest, blob->refData, blob->sizeBytes);
	else
		memset(dest, 0, blob->sizeBytes);
	
	blob->updatedSegmentAddress =
		(blob->originalSegmentAddress & 0xff000000)
		| gWork->size
	;
	gWork->size += blob->sizeBytes;
	
	/*
	fprintf(stderr, "append blob type %d at %08x\n"
		, blob->type, gWork->size - blob->sizeBytes
	);
	*/
	
	return blob->updatedSegmentAddress;
}

static void WorkblobPush(uint8_t alignBytes)
{
	if ((gWorkblob - gWorkblobStack) >= WORKBLOB_STACK_SIZE - 1)
		Die("WorkblobPush() stack overflow");
	
	const uint8_t *data = gWorkblobData;
	if (gWorkblob >= gWorkblobStack)
	{
		data = gWorkblob->refData;
		data += gWorkblob->sizeBytes;
	}
	
	gWorkblob += 1;
	gWorkblob->refData = data;
	gWorkblob->sizeBytes = 0;
	gWorkblob->originalSegmentAddress = gWorkblobSegment;
	gWorkblob->alignBytes = alignBytes;
	gWorkFindAlignment = alignBytes;
}

static uint32_t WorkblobPop(void)
{
	if (gWorkblob < gWorkblobStack)
		Die("WorkblobPop() stack underflow");
	
	gWorkblobAddr = WorkAppendDatablob(gWorkblob);
	gWorkblobAddrEnd = gWorkblobAddr + gWorkblob->sizeBytes;
	if (!gWorkblob->sizeBytes) gWorkblobAddr = 0;
	gWorkblob -= 1;
	
	if (gWorkblob >= gWorkblobStack)
		gWorkFindAlignment = gWorkblob->alignBytes;
	
	return gWorkblobAddr;
}

static void WorkblobSegment(uint8_t segment)
{
	gWorkblobSegment = segment << 24;
}

static void WorkblobPut8(uint8_t data)
{
	((uint8_t*)gWorkblob->refData)[gWorkblob->sizeBytes++] = data;
}

static void WorkblobPut16(uint16_t data)
{
	// align
	while (gWorkblob->sizeBytes & 1)
		WorkblobPut8(0);
	
	WorkblobPut8(data >> 8);
	WorkblobPut8(data);
}

static void WorkblobPut32(uint32_t data)
{
	// align
	while (gWorkblob->sizeBytes & 3)
		WorkblobPut8(0);
	
	WorkblobPut8(data >> 24);
	WorkblobPut8(data >> 16);
	WorkblobPut8(data >> 8);
	WorkblobPut8(data);
}

static void WorkblobPutByteArray(const uint8_t data[], int len)
{
	memcpy(((uint8_t*)gWorkblob->refData) + gWorkblob->sizeBytes, data, len);
	
	gWorkblob->sizeBytes += len;
}

static void WorkblobPutString(const char *data)
{
	WorkblobPutByteArray((void*)data, strlen(data));
}

static void WorkblobAllowDuplicates(bool allow)
{
	gWorkblobAllowDuplicates = allow;
}

static void WorkblobThisExactlyBegin(uint32_t wantThisSize)
{
	gWorkblobExactlyThisSize = wantThisSize;
	gWorkblobExactlyThisSizeStartSize = gWorkblob->sizeBytes;
}

static void WorkblobThisExactlyEnd(void)
{
	while (gWorkblob->sizeBytes
		- gWorkblobExactlyThisSizeStartSize
		< gWorkblobExactlyThisSize
	)
		WorkblobPut8(0);
	
	gWorkblobExactlyThisSize = 0;
}

static void WorkFirstHeader(void)
{
	gWorkblob += 1;
	
	memcpy(gWork->data, gWorkblob->refData, gWorkblob->sizeBytes);
	gWork->size -= gWorkblob->sizeBytes;
	
	gWorkblob -= 1;
}

static void WorkAppendRoomShapeImage(RoomShapeImage image)
{
	WorkblobPut32(Swap32(image.sourceBEU32));
	WorkblobPut32(image.unk_0C);
	WorkblobPut32(Swap32(image.tlutBEU32));
	WorkblobPut16(image.width);
	WorkblobPut16(image.height);
	WorkblobPut8(image.fmt);
	WorkblobPut8(image.siz);
	WorkblobPut16(image.tlutMode);
	WorkblobPut16(image.tlutCount);
}

static uint32_t WorkAppendRoomHeader(struct RoomHeader *header, uint32_t alternateHeaders)
{
	if (header->isBlank)
		return 0;
	
	WorkblobSegment(0x03);
	
	// the header
	WorkblobPush(16);
	
	// alternate headers command
	if (alternateHeaders)
	{
		WorkblobPut32(0x18000000);
		WorkblobPut32(alternateHeaders);
	}
	
	// unhandled commands
	sb_foreach(header->unhandledCommands, { WorkblobPut32(*each); });
	
	// mesh header command
	if (header->displayLists)
	{
		WorkblobPut32(0x0A000000);
		
		// mesh header
		{
			WorkblobPush(4);
			
			WorkblobPut8(header->meshFormat);
			
			switch (header->meshFormat)
			{
				case 0:
					// display list entries
					{
						WorkblobPush(4);
						
						sb_foreach(header->displayLists, {
							WorkblobPut32(Swap32(each->opaBEU32));
							WorkblobPut32(Swap32(each->xluBEU32));
						});
						
						WorkblobPop();
					}
					// header
					WorkblobPut8(sb_count(header->displayLists));
					WorkblobPut32(gWorkblobAddr);
					WorkblobPut32(gWorkblobAddrEnd);
					break;
				
				// maps that use prerendered backgrounds
				case 1:
				{
					int amountType = header->image.base.amountType;
					
					WorkblobPut8(amountType);
					
					// display list entries
					{
						WorkblobPush(4);
						
						sb_foreach(header->displayLists, {
							WorkblobPut32(Swap32(each->opaBEU32));
							WorkblobPut32(Swap32(each->xluBEU32));
						});
						WorkblobPut32(0);
						WorkblobPut32(0);
						
						WorkblobPut32(WorkblobPop());
					}
					
					if (amountType == ROOM_SHAPE_IMAGE_AMOUNT_SINGLE)
					{
						WorkAppendRoomShapeImage(header->image.single.image);
					}
					else if (amountType == ROOM_SHAPE_IMAGE_AMOUNT_MULTI)
					{
						sb_array(RoomShapeImageMultiBgEntry, backgrounds) = header->image.multi.backgrounds;
						
						WorkblobPut8(sb_count(backgrounds));
						WorkblobPush(4);
						sb_foreach(backgrounds, {
							WorkblobThisExactlyBegin(0x1C);
							fprintf(stderr, "     unk00 = %04x\n", each->unk_00);
							fprintf(stderr, "bgCamIndex = %04x\n", each->bgCamIndex);
							WorkblobPut16(each->unk_00);
							WorkblobPut8(each->bgCamIndex);
							WorkAppendRoomShapeImage(each->image);
							WorkblobThisExactlyEnd();
						})
						WorkblobPut32(WorkblobPop());
					}
					break;
				}
				
				case 2:
					// display list entries
					{
						WorkblobPush(4);
						
						sb_foreach(header->displayLists, {
							WorkblobPut16(each->center.x);
							WorkblobPut16(each->center.y);
							WorkblobPut16(each->center.z);
							WorkblobPut16(each->radius);
							WorkblobPut32(Swap32(each->opaBEU32));
							WorkblobPut32(Swap32(each->xluBEU32));
						});
						
						WorkblobPop();
					}
					// header
					WorkblobPut8(sb_count(header->displayLists));
					WorkblobPut32(gWorkblobAddr);
					WorkblobPut32(gWorkblobAddrEnd);
					break;
			}
			
			WorkblobPop();
		}
		
		WorkblobPut32(gWorkblobAddr);
	}
	
	// object list
	if (header->objects)
	{
		WorkblobPut32(0x0B000000 | (sb_count(header->objects) << 16));
		
		WorkblobPush(4);
		
		sb_foreach(header->objects, {
			WorkblobPut16(*each);
		});
		
		WorkblobPut32(WorkblobPop());
	}
	
	// actor list
	if (header->instances)
	{
		WorkblobPut32(0x01000000 | (sb_count(header->instances) << 16));
		
		WorkblobPush(4);
		
		sb_foreach(header->instances, {
			WorkblobPut16(each->id);
			WorkblobPut16(rintf(each->pos.x));
			WorkblobPut16(rintf(each->pos.y));
			WorkblobPut16(rintf(each->pos.z));
			WorkblobPut16(each->xrot);
			WorkblobPut16(each->yrot);
			WorkblobPut16(each->zrot);
			WorkblobPut16(each->params);
		});
		
		WorkblobPut32(WorkblobPop());
	}
	
	// end header command
	WorkblobPut32(0x14000000);
	WorkblobPut32(0x00000000);
	
	return WorkblobPop();
}

static uint32_t WorkAppendSceneHeader(struct Scene *scene, struct SceneHeader *header, uint32_t alternateHeaders)
{
	// this is a bandaid solution for z64rom, which assumes
	// the alternate header block ends where the next block
	// begins; this hack guarantees one small block that is
	// guaranteed to be referenced by the scene header will
	// not be optimized away using duplicate detection
	bool z64romHack = false;
	
	if (header->isBlank)
		return 0;
	
	WorkblobSegment(0x02);
	
	// the header
	WorkblobPush(16);
	
	// alternate headers command
	if (alternateHeaders)
	{
		WorkblobPut32(0x18000000);
		WorkblobPut32(alternateHeaders);
		z64romHack = true;
	}
	
	// unhandled commands
	sb_foreach(header->unhandledCommands, { WorkblobPut32(*each); });
	
	// room list command
	if (header->numRooms)
	{
		int numRooms = header->numRooms;
		
		WorkblobPut32(0x04000000 | (numRooms << 16));
		
		// room list
		WorkblobAllowDuplicates(z64romHack);
		{
			WorkblobPush(4);
			
			for (int i = 0; i < numRooms; ++i)
			{
				char roomName[16];
				
				sprintf(roomName, "room%04x", i);
				
				WorkblobPutString(roomName);
			}
			
			WorkblobPop();
		}
		WorkblobAllowDuplicates(false);
		
		WorkblobPut32(gWorkblobAddr);
	}
	
	// environment list command
	if (header->lights)
	{
		WorkblobPut32(0x0F000000 | (sb_count(header->lights) << 16));
		
		WorkblobPush(4);
		sb_foreach(header->lights, {
			WorkblobPutByteArray((void*)&each->ambient, 3);
			
			WorkblobPutByteArray((void*)&each->diffuse_a_dir, 3);
			WorkblobPutByteArray((void*)&each->diffuse_a, 3);
			
			WorkblobPutByteArray((void*)&each->diffuse_b_dir, 3);
			WorkblobPutByteArray((void*)&each->diffuse_b, 3);
			
			WorkblobPutByteArray((void*)&each->fog, 3);
			
			WorkblobPut16(each->fog_near);
			WorkblobPut16(each->fog_far);
		});
		WorkblobPop();
		
		WorkblobPut32(gWorkblobAddr);
	}
	
	// animated materials
	if (header->mm.sceneSetupData)
	{
		WorkblobPut32(0x1A000000);
		
		AnimatedMaterialToWorkblob(
			header->mm.sceneSetupData
			, WorkblobPush
			, WorkblobPop
			, WorkblobPut8
			, WorkblobPut16
			, WorkblobPut32
		);
		
		WorkblobPut32(gWorkblobAddr);
	}
	
	// path list
	if (header->paths)
	{
		WorkblobPut32(0x0D000000 | (sb_count(header->paths) << 16));
		
		WorkblobPush(4);
		
		sb_foreach(header->paths, {
			WorkblobPut8(sb_count(each->points));
			WorkblobPut8(each->additionalPathIndex);
			WorkblobPut16(each->customValue);
			typeof(each->points) points = each->points;
			WorkblobPush(2);
			sb_foreach(points, {
				WorkblobPut16(each->x);
				WorkblobPut16(each->y);
				WorkblobPut16(each->z);
			});
			WorkblobPut32(WorkblobPop());
		});
		
		WorkblobPut32(WorkblobPop());
	}
	
	// spawn points
	if (header->spawns)
	{
		// NOTE: 0x06 command must precede 0x00 command
		WorkblobPut32(0x06000000);
		WorkblobPush(4);
		sb_foreach(header->spawns, {
			WorkblobPut8(eachIndex);
			WorkblobPut8(each->spawnpoint.room);
		});
		WorkblobPop();
		WorkblobPut32(gWorkblobAddr);
		
		WorkblobPut32(0x00000000 | (sb_count(header->spawns) << 16));
		WorkblobPush(4);
		sb_foreach(header->spawns, {
			WorkblobPut16(each->id);
			WorkblobPut16(rintf(each->pos.x));
			WorkblobPut16(rintf(each->pos.y));
			WorkblobPut16(rintf(each->pos.z));
			WorkblobPut16(each->xrot);
			WorkblobPut16(each->yrot);
			WorkblobPut16(each->zrot);
			WorkblobPut16(each->params);
		});
		WorkblobPop();
		WorkblobPut32(gWorkblobAddr);
	}
	
	// collision
	if (scene->collisions)
	{
		WorkblobPut32(0x03000000);
		
		CollisionHeaderToWorkblob(scene->collisions);
		
		WorkblobPut32(gWorkblobAddr);
	}
	
	// exits
	if (header->exits)
	{
		WorkblobPut32(0x13000000);
		
		WorkblobPush(4);
		sb_foreach(header->exits, { WorkblobPut16(*each); });
		WorkblobPop();
		
		WorkblobPut32(gWorkblobAddr);
	}
	
	// doorways
	if (header->doorways)
	{
		WorkblobPut32(0x0E000000 | (sb_count(header->doorways) << 16));
		
		WorkblobPush(4);
		sb_foreach(header->doorways, {
			WorkblobPut8(each->doorway.frontRoom);
			WorkblobPut8(each->doorway.frontCamera);
			WorkblobPut8(each->doorway.backRoom);
			WorkblobPut8(each->doorway.backCamera);
			WorkblobPut16(each->id);
			WorkblobPut16(rintf(each->pos.x));
			WorkblobPut16(rintf(each->pos.y));
			WorkblobPut16(rintf(each->pos.z));
			WorkblobPut16(each->yrot);
			WorkblobPut16(each->params);
		});
		WorkblobPop();
		
		WorkblobPut32(gWorkblobAddr);
	}
	
	// cutscenes
	if (header->cutsceneOot)
	{
		WorkblobPut32(0x17000000);
		
		CutsceneOotToWorkblob(
			header->cutsceneOot
			, WorkblobPush
			, WorkblobPop
			, WorkblobPut8
			, WorkblobPut16
			, WorkblobPut32
			, WorkblobThisExactlyBegin
			, WorkblobThisExactlyEnd
		);
		
		WorkblobPut32(gWorkblobAddr);
	}
	
	// cutscene lists
	if (sb_count(header->cutsceneListMm))
	{
		WorkblobPut32(0x17000000 | (sb_count(header->cutsceneListMm) << 16));
		
		CutsceneListMmToWorkblob(
			header->cutsceneListMm
			, WorkblobPush
			, WorkblobPop
			, WorkblobPut8
			, WorkblobPut16
			, WorkblobPut32
			, WorkblobThisExactlyBegin
			, WorkblobThisExactlyEnd
		);
		
		WorkblobPut32(gWorkblobAddr);
	}
	
	// actor cutscene camera data
	if (sb_count(header->actorCsCamInfo))
	{
		WorkblobPut32(0x02000000 | (sb_count(header->actorCsCamInfo) << 16));
		
		WorkblobPush(4);
		sb_foreach(header->actorCsCamInfo, {
			sb_array(Vec3s, data) = each->actorCsCamFuncData;
			WorkblobPut16(each->setting);
			WorkblobPut16(sb_count(data));
			WorkblobPush(2);
				sb_foreach(data, {
					WorkblobPut16(each->x);
					WorkblobPut16(each->y);
					WorkblobPut16(each->z);
				});
			WorkblobPut32(WorkblobPop());
		});
		
		WorkblobPut32(WorkblobPop());
	}
	
	// actor cutscenes
	if (sb_count(header->actorCutscenes))
	{
		WorkblobPut32(0x1B000000 | (sb_count(header->actorCutscenes) << 16));
		
		WorkblobPush(2);
		sb_foreach(header->actorCutscenes, {
			WorkblobPut16(each->priority);
			WorkblobPut16(each->length);
			WorkblobPut16(each->csCamId);
			WorkblobPut16(each->scriptIndex);
			WorkblobPut16(each->additionalCsId);
			WorkblobPut8(each->endSfx);
			WorkblobPut8(each->customValue);
			WorkblobPut16(each->hudVisibility);
			WorkblobPut8(each->endCam);
			WorkblobPut8(each->letterboxSize);
		});
		
		WorkblobPut32(WorkblobPop());
	}
	
	// end header command
	WorkblobPut32(0x14000000);
	WorkblobPut32(0x00000000);
	
	return WorkblobPop();
}

void RoomToFilename(struct Room *room, const char *filename)
{
	if (filename == 0)
		filename = room->file->filename;
	else {
		free(room->file->filename);
		room->file->filename = Strdup(filename);
	}
	
	fprintf(stderr, "write room '%s'\n", filename);
	
	// prepare fresh work buffer
	WorkReady();
	WorkAppendDatablob(FIRST_HEADER);
	
	// write output file
	FileToFilename(gWork, filename);
	
	// append all non-mesh blobs first
	datablob_foreach_filter(room->blobs, != DATA_BLOB_TYPE_MESH, {
		WorkAppendDatablob(each);
		DataBlobApplyUpdatedSegmentAddresses(each);
	});
	
	// append mesh blobs
	datablob_foreach_filter(room->blobs, == DATA_BLOB_TYPE_MESH, {
		WorkAppendDatablob(each);
		DataBlobApplyUpdatedSegmentAddresses(each);
	});
	
	gWorkFindAlignment = 4; // more lenient alignment after meshes written
	
	// append room headers
	{
		// alternate headers
		uint32_t alternateHeadersAddr = 0;
		sb_array(uint32_t, alternateHeaders) = 0;
		sb_foreach(room->headers, {
			if (eachIndex)
				sb_push(alternateHeaders, WorkAppendRoomHeader(each, 0));
		});
		if (sb_count(alternateHeaders))
		{
			WorkblobPush(4);
			sb_foreach(alternateHeaders, { WorkblobPut32(*each); });
			PAD_ALTERNATE_HEADERS(alternateHeaders, MIN_ALTERNATE_HEADERS)
			alternateHeadersAddr = WorkblobPop();
		}
		
		// main header
		WorkAppendRoomHeader(&room->headers[0], alternateHeadersAddr);
		WorkFirstHeader();
		sb_free(alternateHeaders);
	}
	
	// write output file
	FileToFilename(gWork, filename);
}

void SceneToFilename(struct Scene *scene, const char *filename)
{
	struct DataBlob *blob;
	bool useOriginalFilenames = false;
	static char append[2048];
	
	if (filename == 0)
	{
		filename = scene->file->filename;
		useOriginalFilenames = true;
	}
	else
	{
		// append zscene extension if not present
		if (!strrchr(filename, '.') // no period
			|| !(strcpy(append, strrchr(filename, '.') + 1)
				&& strlen(append) == 6 // zscene
				&& StrToLower(append)
				&& !strcmp(append, "zscene")
			)
		)
		{
			snprintf(append, sizeof(append), "%s.zscene", filename);
			filename = append;
		}
		
		free(scene->file->filename);
		scene->file->filename = Strdup(filename);
	}
	
	fprintf(stderr, "write scene '%s'\n", filename);
	
	// make sure everything is zero
	for (blob = scene->blobs; blob; blob = blob->next)
		blob->updatedSegmentAddress = 0;
	sb_foreach(scene->rooms, {
		for (blob = each->blobs; blob; blob = blob->next)
			blob->updatedSegmentAddress = 0;
	});
	
	// prepare fresh work buffer
	WorkReady();
	WorkAppendDatablob(FIRST_HEADER);
	
	// append all non-mesh blobs first
	datablob_foreach_filter(scene->blobs, != DATA_BLOB_TYPE_MESH, {
		WorkAppendDatablob(each);
		DataBlobApplyUpdatedSegmentAddresses(each);
	});
	
	// append mesh blobs
	datablob_foreach_filter(scene->blobs, == DATA_BLOB_TYPE_MESH, {
		WorkAppendDatablob(each);
		//fprintf(stderr, "mesh appended %08x -> %08x\n", each->originalSegmentAddress, each->updatedSegmentAddress);
		DataBlobApplyUpdatedSegmentAddresses(each);
	});
	
	gWorkFindAlignment = 4; // more lenient alignment after meshes written
	
	// append scene headers
	{
		// alternate headers
		uint32_t alternateHeadersAddr = 0;
		sb_array(uint32_t, alternateHeaders) = 0;
		sb_foreach(scene->headers, {
			if (eachIndex)
				sb_push(alternateHeaders, WorkAppendSceneHeader(scene, each, 0));
		});
		if (sb_count(alternateHeaders))
		{
			WorkblobPush(4);
			sb_foreach(alternateHeaders, { WorkblobPut32(*each); });
			PAD_ALTERNATE_HEADERS(alternateHeaders, MIN_ALTERNATE_HEADERS)
			alternateHeadersAddr = WorkblobPop();
		}
		
		// main header
		WorkAppendSceneHeader(scene, &scene->headers[0], alternateHeadersAddr);
		WorkFirstHeader();
		sb_free(alternateHeaders);
	}
	
	// write output file
	FileToFilename(gWork, filename);
	
	// write rooms
	sb_foreach(scene->rooms, {
		if (useOriginalFilenames)
		{
			RoomToFilename(each, 0);
		}
		else
		{
			static char fn[2048];
			char *roomNameBuf = strcpy(fn, filename);
			
			// TODO: employ DRY on this, copy-pasted from SceneFromFilenamePredictRooms()
			char *lastSlash = MAX(strrchr(roomNameBuf, '\\'), strrchr(roomNameBuf, '/'));
			if (!lastSlash)
				lastSlash = roomNameBuf;
			else
				lastSlash += 1;
			lastSlash = MAX(lastSlash, strstr(lastSlash, "_scene"));
			if (*lastSlash == '_')
				lastSlash += 1;
			else
				lastSlash = strcpy(strrchr(lastSlash, '.'), "_") + 1;
			
			sprintf(lastSlash, "room_%d.zmap", eachIndex);
			
			RoomToFilename(each, fn);
		}
	});
	
	// restore original segment addresses for everything
	sb_foreach(scene->rooms, {
		blob = each->blobs;
		datablob_foreach(blob, {
			DataBlobApplyOriginalSegmentAddresses(each);
		});
	});
	datablob_foreach(scene->blobs, {
		DataBlobApplyOriginalSegmentAddresses(each);
	});
}

void CollisionHeaderToWorkblob(CollisionHeader *header)
{
	WorkblobPush(4);
	
	WorkblobPut16(header->minBounds.x);
	WorkblobPut16(header->minBounds.y);
	WorkblobPut16(header->minBounds.z);
	
	WorkblobPut16(header->maxBounds.x);
	WorkblobPut16(header->maxBounds.y);
	WorkblobPut16(header->maxBounds.z);
	
	WorkblobPut16(header->numVertices);
	WorkblobPush(4);
	for (int i = 0; i < header->numVertices; ++i) {
		Vec3s vtx = header->vtxList[i];
		WorkblobPut16(vtx.x);
		WorkblobPut16(vtx.y);
		WorkblobPut16(vtx.z);
	} WorkblobPut32(WorkblobPop());
	
	WorkblobPut16(header->numPolygons);
	WorkblobPush(4);
	for (int i = 0; i < header->numPolygons; ++i) {
		CollisionPoly poly = header->polyList[i];
		WorkblobPut16(poly.type);
		WorkblobPut16(poly.vtxData[0]);
		WorkblobPut16(poly.vtxData[1]);
		WorkblobPut16(poly.vtxData[2]);
		WorkblobPut16(poly.normal.x);
		WorkblobPut16(poly.normal.y);
		WorkblobPut16(poly.normal.z);
		WorkblobPut16(poly.dist);
	} WorkblobPut32(WorkblobPop());
	
	WorkblobPush(4);
	for (int i = 0; i < header->numSurfaceTypes; ++i) {
		SurfaceType type = header->surfaceTypeList[i];
		WorkblobPut32(type.w0);
		WorkblobPut32(type.w1);
	} WorkblobPut32(WorkblobPop());
	
	WorkblobPush(4);
	for (int i = 0; i < sb_count(header->bgCamList); ++i) {
		BgCamInfo cam = header->bgCamList[i];
		WorkblobPut16(cam.setting);
		WorkblobPut16(cam.count);
		if (cam.dataAddrResolved) {
			WorkblobPush(2); {
				/*
				BgCamFuncData data = cam.bgCamFuncData;
				WorkblobPut16(data.pos.x);
				WorkblobPut16(data.pos.y);
				WorkblobPut16(data.pos.z);
				WorkblobPut16(data.rot.x);
				WorkblobPut16(data.rot.y);
				WorkblobPut16(data.rot.z);
				WorkblobPut16(data.fov);
				WorkblobPut16(data.flags);
				WorkblobPut16(data.unused);
				*/
				Vec3s *data = (void*)cam.data;
				for (int k = 0; k < cam.count; ++k, ++data)
				{
					WorkblobPut16(data->x);
					WorkblobPut16(data->y);
					WorkblobPut16(data->z);
				}
			} WorkblobPut32(WorkblobPop());
		} else
			WorkblobPut32(cam.dataAddr);
	} WorkblobPut32(WorkblobPop());
	
	WorkblobPut16(header->numWaterBoxes);
	WorkblobPush(4);
	for (int i = 0; i < header->numWaterBoxes; ++i) {
		WaterBox waterbox = header->waterBoxes[i];
		WorkblobPut16(waterbox.xMin);
		WorkblobPut16(waterbox.ySurface);
		WorkblobPut16(waterbox.zMin);
		WorkblobPut16(waterbox.xLength);
		WorkblobPut16(waterbox.zLength);
		WorkblobPut16(waterbox.unused);
		WorkblobPut32(waterbox.properties);
	} WorkblobPut32(WorkblobPop());
	
	WorkblobPop();
}
