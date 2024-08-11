//
// datablobs.h
//
// for storing and manipulating data blobs
//

#ifndef DATABLOBS_H_INCLUDED
#define DATABLOBS_H_INCLUDED

#include <stdint.h>
#include "stretchy_buffer.h"

typedef void (*DataBlobCallbackFunc)(void *udata, uint32_t sizeBytes);

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
	DATA_BLOB_TYPE_UNSET,
	DATA_BLOB_TYPE_MESH,
	DATA_BLOB_TYPE_VERTEX,
	DATA_BLOB_TYPE_MATRIX,
	DATA_BLOB_TYPE_TEXTURE,
	DATA_BLOB_TYPE_PALETTE,
	DATA_BLOB_TYPE_GENERIC,
	DATA_BLOB_TYPE_EOF, // end-of-file marker
	DATA_BLOB_TYPE_COUNT,
};

enum DataBlobSubtype
{
	DATA_BLOB_SUBTYPE_UNSET,
	DATA_BLOB_SUBTYPE_FLIPBOOK,
	DATA_BLOB_SUBTYPE_COUNT,
};

struct DataBlobCallback
{
	DataBlobCallbackFunc func;
	void *udata;
};

struct DataBlobPending
{
	uint32_t segAddr;
	uint32_t sizeBytes;
	sb_array(struct DataBlobCallback, postsort);
};

struct DataBlob
{
	// TODO XXX refData as first member prevents an #include in texanim.c, refactor later
	const void *refData; // is a reference, do not free
	void *udata;
	struct DataBlob *next;
	struct DataBlobPending callbacks;
	const void *refDataFileEnd;
	uint32_t originalSegmentAddress;
	uint32_t updatedSegmentAddress;
	uint32_t sizeBytes;
	uint8_t alignBytes;
	enum DataBlobType type;
	enum DataBlobSubtype subtype;
	bool ownsRefData;
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
			bool isJfif;
			uintptr_t glTexture;
			uint8_t flipbookSegment;
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
struct DataBlob *DataBlobNew(
	const void *refData
	, uint32_t sizeBytes
	, uint32_t segmentAddr
	, enum DataBlobType type
	, struct DataBlob *next
	, void *ref
);
struct DataBlob *DataBlobPush(
	struct DataBlob *listHead
	, const void *refData
	, uint32_t sizeBytes
	, uint32_t segmentAddr
	, enum DataBlobType type
	, void *ref
);
void DataBlobSegmentsPopulateFromRoomMesh(
	struct DataBlob **seg2head
	, struct DataBlob **seg3head
	, const void *seg2
	, const void *seg3
);
void DataBlobListTouchBlob(struct DataBlob **listHead, struct DataBlob *blob);
void DataBlobRemoveRef(struct DataBlob *blob, void *ref);
void DatablobFree(struct DataBlob *blob);
void DatablobFreeList(struct DataBlob *listHead);
void DataBlobListRemoveBlankEntries(struct DataBlob **listHead);
struct DataBlob *DataBlobListFindBlobWithOriginalSegmentAddress(struct DataBlob *listHead, uint32_t originalSegmentAddress);
void DataBlobSegmentSetup(int segmentIndex, const void *data, const void *dataEnd, struct DataBlob *head);
struct DataBlobSegment *DataBlobSegmentGet(int segmentIndex);
struct DataBlob *DataBlobSegmentGetHead(int segmentIndex);
bool DataBlobSegmentContainsSegAddr(int segmentIndex, uint32_t originalSegmentAddress);
void DataBlobSegmentsPopulateFromMesh(uint32_t segAddr, void *originator);
void DataBlobSegmentsPopulateFromMeshNew(uint32_t segAddr, void *originator);
void DataBlobSegmentsPopulateFromRoomShapeImage(void *roomShapeImage);
void DataBlobPrint(struct DataBlob *blob);
void DataBlobPrintAll(struct DataBlob *blobs);
const void *DataBlobSegmentAddressToRealAddress(uint32_t segAddr);
void DataBlobApplyUpdatedSegmentAddresses(struct DataBlob *blob);
void DataBlobApplyOriginalSegmentAddresses(struct DataBlob *blob);

#endif
