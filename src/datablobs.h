//
// datablobs.h
//
// for storing and manipulating data blobs
//

#ifndef DATABLOBS_H_INCLUDED
#define DATABLOBS_H_INCLUDED

#include <stdint.h>

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
	uint32_t originalSegmentAddress;
	uint32_t updatedSegmentAddress;
	uint32_t sizeBytes;
	enum DataBlobType type;
	
	union {
		struct {
			int w;
			int h;
			int siz;
			int fmt;
			struct DataBlob *pal;
		} texture;
	} data;
};

struct DataBlobSegment
{
	struct DataBlob *head;
	const void *data;
};

struct TextureBlob
{
	struct DataBlob *data;
	struct File *file;
};
#define TextureBlobStack(DATA, FILE) (struct TextureBlob){ DATA, FILE}

// functions
void DataBlobSegmentsPopulateFromRoomMesh(
	struct DataBlob **seg2head
	, struct DataBlob **seg3head
	, const void *seg2
	, const void *seg3
);
void DataBlobSegmentSetup(int segmentIndex, const void *data, struct DataBlob *head);
struct DataBlob *DataBlobSegmentGetHead(int segmentIndex);
void DataBlobSegmentsPopulateFromMesh(uint32_t segAddr);
void DataBlobPrint(struct DataBlob *blob);
void DataBlobPrintAll(struct DataBlob *blobs);
const void *DataBlobSegmentAddressToRealAddress(uint32_t segAddr);

#endif
