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
};

struct DataBlob
{
	struct DataBlob *next;
	const void *refData; // is a reference, do not free
	uint32_t originalSegmentAddress;
	uint32_t updatedSegmentAddress;
	uint32_t sizeBytes;
	enum DataBlobType type;
};

struct DataBlobSegment
{
	struct DataBlob *head;
	const void *data;
};

#endif
