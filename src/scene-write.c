//
// scene-write.c
//
// scene/room file writer
//

#include <stdio.h>

#include "misc.h"

static struct File *gWork = 0;
static struct DataBlob *gWorkblob = 0;
static uint32_t gWorkblobAddr = 0;
static uint8_t *gWorkblobData = 0;
#define WORKBUF_SIZE (1024 * 1024 * 4) // 4mib is generous
#define FIRST_HEADER_SIZE 0x100 // sizeBytes of first header in file
#define FIRST_HEADER &(struct DataBlob){ .sizeBytes = FIRST_HEADER_SIZE }

static void WorkReady(void)
{
	if (!gWork)
		gWork = FileNew("work", WORKBUF_SIZE);
	
	if (!gWorkblob)
	{
		gWorkblobData = Calloc(1, WORKBUF_SIZE);
		gWorkblob = Calloc(1, sizeof(*gWorkblob));
		gWorkblob->refData = gWorkblobData;
	}
	
	gWork->size = 0;
	gWorkblob->sizeBytes = 0;
}

static uint32_t WorkFindDatablob(struct DataBlob *blob)
{
	const uint8_t *needle = blob->refData;
	const uint8_t *haystack = gWork->data;
	
	if (!needle || gWork->size <= FIRST_HEADER_SIZE)
		return 0;
	
	const uint8_t *match = Memmem(
		haystack + FIRST_HEADER_SIZE
		, gWork->size - FIRST_HEADER_SIZE
		, needle
		, blob->sizeBytes
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
	void *dest = ((uint8_t*)gWork->data) + gWork->size;
	uint32_t addr;
	
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

static void WorkblobBegin(void)
{
	gWorkblob->sizeBytes = 0;
}

static uint32_t WorkblobEnd(void)
{
	gWorkblobAddr = WorkAppendDatablob(gWorkblob);
	
	return gWorkblobAddr;
}

static void WorkblobPut8(uint8_t data)
{
	gWorkblobData[gWorkblob->sizeBytes++] = data;
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
	memcpy(gWorkblobData + gWorkblob->sizeBytes, data, len);
	
	gWorkblob->sizeBytes += len;
}

static void WorkblobPutString(const char *data)
{
	WorkblobPutByteArray((void*)data, strlen(data));
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
