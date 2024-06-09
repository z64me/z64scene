//
// datablobs.h
//
// for storing and manipulating data blobs
//

#ifndef DATABLOBS_H_INCLUDED
#define DATABLOBS_H_INCLUDED

#include <stdint.h>
#include "stretchy_buffer.h"

#define datablob_foreach(BLOBS, CODE) \
	for (struct DataBlob *each = BLOBS; each; each = each->next) { \
		CODE \
	}

#define datablob_foreach_filter(BLOBS, FILTER, CODE) \
	for (struct DataBlob *each = BLOBS; each; each = each->next) { \
		if (each->type FILTER) { \
			CODE \
		} \
	}

enum DataBlobType
{
	DATA_BLOB_TYPE_MESH,
	DATA_BLOB_TYPE_VERTEX,
	DATA_BLOB_TYPE_MATRIX,
	DATA_BLOB_TYPE_TEXTURE,
	DATA_BLOB_TYPE_PALETTE,
	DATA_BLOB_TYPE_GENERIC,
	DATA_BLOB_TYPE_COUNT,
};

struct DataBlob
{
	struct DataBlob *next;
	const void *refData; // is a reference, do not free
	const void *refDataFileEnd;
	uint32_t originalSegmentAddress;
	uint32_t updatedSegmentAddress;
	uint32_t sizeBytes;
	uint8_t alignBytes;
	enum DataBlobType type;
	sb_array(void*, refs); // references to this datablob
	
	union {
		struct {
			int w;
			int h;
			int siz;
			int fmt;
			int lineSize;
			int sizeBytesClamped;
			struct DataBlob *pal;
		} texture;
	} data;
};

struct DataBlobSegment
{
	struct DataBlob *head;
	const void *data;
	const void *dataEnd;
};

struct TextureBlob
{
	struct DataBlob *data;
	struct File *file;
};
#define TextureBlobStack(DATA, FILE) (struct TextureBlob){ DATA, FILE }

// functions
void DataBlobSegmentsPopulateFromRoomMesh(
	struct DataBlob **seg2head
	, struct DataBlob **seg3head
	, const void *seg2
	, const void *seg3
);
void DataBlobSegmentSetup(int segmentIndex, const void *data, const void *dataEnd, struct DataBlob *head);
struct DataBlobSegment *DataBlobSegmentGet(int segmentIndex);
struct DataBlob *DataBlobSegmentGetHead(int segmentIndex);
void DataBlobSegmentsPopulateFromMesh(uint32_t segAddr, void *originator);
void DataBlobSegmentsPopulateFromMeshNew(uint32_t segAddr, void *originator);
void DataBlobPrint(struct DataBlob *blob);
void DataBlobPrintAll(struct DataBlob *blobs);
const void *DataBlobSegmentAddressToRealAddress(uint32_t segAddr);
void DataBlobApplyUpdatedSegmentAddresses(struct DataBlob *blob);
void DataBlobApplyOriginalSegmentAddresses(struct DataBlob *blob);

#endif
