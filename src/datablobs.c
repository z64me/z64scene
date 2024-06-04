//
// datablobs.c
//
// for storing and manipulating data blobs
//

#include <stdlib.h>
#include <string.h>

#include "datablobs.h"

static struct DataBlobSegment gSegments[16];

struct DataBlob *DataBlobNew(
	const void *refData
	, uint32_t sizeBytes
	, uint32_t segmentAddr
	, enum DataBlobType type
	, struct DataBlob *next
)
{
	struct DataBlob *blob = malloc(sizeof(*blob));
	
	*blob = (struct DataBlob) {
		.next = next
		, .refData = refData
		, .originalSegmentAddress = segmentAddr
		, .sizeBytes = sizeBytes
		, .type = type
	};
	
	return blob;
}

// listHead = DataBlobPush(listHead, ...)
struct DataBlob *DataBlobPush(
	struct DataBlob *listHead
	, const void *refData
	, uint32_t sizeBytes
	, uint32_t segmentAddr
	, enum DataBlobType type
)
{
	for (struct DataBlob *each = listHead; each; each = each->next)
	{
		if (each->originalSegmentAddress == segmentAddr)
		{
			// it's possible for blocks to be coalesced
			if (sizeBytes > each->sizeBytes)
				each->sizeBytes = sizeBytes;
			
			return listHead;
		}
	}
	
	// prepend so addresses can later be resolved in reverse order
	return DataBlobNew(refData, sizeBytes, segmentAddr, type, listHead);
}

struct DataBlob *DataBlobFindBySegAddr(struct DataBlob *listHead, uint32_t segAddr)
{
	for (struct DataBlob *each = listHead; each; each = each->next)
	{
		if (each->originalSegmentAddress == segAddr)
			return each;
	}
	
	return 0;
}

void DataBlobSegmentClearAll(void)
{
	memset(gSegments, 0, sizeof(gSegments));
}

void DataBlobSegmentSetup(int segmentIndex, const void *data, struct DataBlob *head)
{
	struct DataBlobSegment *seg = &gSegments[segmentIndex];
	
	seg->head = head;
	seg->data = data;
}

struct DataBlob *DataBlobSegmentGetHead(int segmentIndex)
{
	return gSegments[segmentIndex].head;
}

void DataBlobSegmentsPopulateFromMesh(const void *mesh)
{
}

void DataBlobSegmentsPopulateFromRoomMesh(
	struct DataBlob **seg2head
	, struct DataBlob **seg3head
	, const void *seg2
	, const void *seg3
)
{
	// initial run
	if (!*seg2head)
		DataBlobSegmentSetup(2, seg2, *seg2head);
	
	DataBlobSegmentSetup(3, seg3, *seg3head);
	
	// TODO for each dlist
	
	*seg2head = DataBlobSegmentGetHead(2);
	*seg3head = DataBlobSegmentGetHead(3);
}
