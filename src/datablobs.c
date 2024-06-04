//
// datablobs.c
//
// for storing and manipulating data blobs
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "datablobs.h"

#define F3DEX_GBI_2
#include "gbi.h"

// gbi extras
/* dl push flag */
#define G_DL_PUSH   0
#define G_DL_NOPUSH 1
/* 10.2 fixed point */
typedef uint16_t qu102_t;
#define qu102_I(x) \
    ((x) >> 2)
#define G_SIZ_BYTES(siz) (G_SIZ_BITS(siz) / 8)
#define ALIGN8(x) (((x) + 7) & ~7)

#define READ_32_BE(BYTES, OFFSET) ( \
	(((const uint8_t*)(BYTES))[OFFSET + 0] << 24) | \
	(((const uint8_t*)(BYTES))[OFFSET + 1] << 16) | \
	(((const uint8_t*)(BYTES))[OFFSET + 2] <<  8) | \
	(((const uint8_t*)(BYTES))[OFFSET + 3] <<  0) \
)

#define ARRLEN(X) (sizeof(X) / sizeof(*(X)))
#define FALLTHROUGH __attribute__((fallthrough))
#define SIZEOF_GFX 8
#define SIZEOF_MTX 0x40
#define SIZEOF_VTX 0x10
#define SHIFTL(v, s, w) \
	((uint32_t)(((uint32_t)(v) & ((0x01 << (w)) - 1)) << (s)))
#define SHIFTR(v, s, w) \
	((uint32_t)(((uint32_t)(v) >> (s)) & ((0x01 << (w)) - 1)))

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

struct DataBlobSegment *DataBlobSegmentGet(int segmentIndex)
{
	if (segmentIndex >= ARRLEN(gSegments))
		return 0;
	
	return &gSegments[segmentIndex];
}

struct DataBlob *DataBlobSegmentGetHead(int segmentIndex)
{
	struct DataBlobSegment *segment = DataBlobSegmentGet(segmentIndex);
	
	if (!segment)
		return 0;
	
	return segment->head;
}

void DataBlobSegmentPush(
	const void *refData
	, uint32_t sizeBytes
	, uint32_t segmentAddr
	, enum DataBlobType type
)
{
	struct DataBlobSegment *seg = DataBlobSegmentGet(segmentAddr >> 24);
	
	if (!seg)
		return;
	
	seg->head = DataBlobPush(seg->head, refData, sizeBytes, segmentAddr, type);
}

const void *DataBlobSegmentAddressToRealAddress(uint32_t segAddr)
{
	// skip unpopulated segments
	if ((segAddr >> 24) >= ARRLEN(gSegments)
		|| gSegments[segAddr >> 24].data == 0
	)
		return 0;
	
	return ((const uint8_t*)gSegments[segAddr >> 24].data) + (segAddr & 0x00ffffff);
}

// adapted from DisplayList_Copy() from Tharo's dlcopy for this
// https://github.com/Thar0/dlcopy/
// DisplayList_Copy (ZObj* obj1, segaddr_t segAddr, ZObj* obj2, segaddr_t* newSegAddr)
void DataBlobSegmentsPopulateFromMesh(uint32_t segAddr)
{
	// skip unpopulated segments
	if ((segAddr >> 24) >= ARRLEN(gSegments)
		|| gSegments[segAddr >> 24].data == 0
	)
		return;
	
	// last 8 commands
	int i = 0;
	uint8_t history[8] = { G_NOOP };
	const void *realAddr;
	
#define HISTORY_GET(n) \
	history[(i - 1 - (n) < 0) ? (i - 1 - (n) + ARRLEN(history)) : (i - 1 - (n))]
	
	// texture engine state tracker
	static struct {
		int fmt;
		int siz;
		uint32_t width;
		uint32_t dram;
	} timg = { 0 };
	
	static struct {
		// SetTile
		int fmt;
		int siz;
		int line;
		uint16_t tmem;
		int pal;
		int cms;
		int cmt;
		int masks;
		int maskt;
		int shifts;
		int shiftt;
		// SetTileSize
		qu102_t uls;
		qu102_t ult;
		qu102_t lrs;
		qu102_t lrt;
	} tileDescriptors[8] = { 0 };
	
	bool exit = false;
	for (const uint8_t *data = DataBlobSegmentAddressToRealAddress(segAddr); !exit; )
	{
		size_t cmdlen = SIZEOF_GFX;
		uint32_t w0 = READ_32_BE(data, 0);
		uint32_t w1 = READ_32_BE(data, 4);
		
		int cmd = w0 >> 24;
		switch (cmd)
		{
			/*
			 * These commands contain pointers, except G_ENDDL which is here for convenience
			 */
			
			case G_DL:
				// recursively copy called display lists
				DataBlobSegmentsPopulateFromMesh(w1);
				// if not branchlist, carry on
				if (SHIFTR(w0, 16, 8) == G_DL_PUSH)
					break;
				// if branchlist, exit
				FALLTHROUGH;
			case G_ENDDL:
				// exit
				exit = true;
				break;
			
			case G_MOVEMEM:
				if ((realAddr = DataBlobSegmentAddressToRealAddress(w1))) {
					uint32_t len = SHIFTR(w0, 19,  5) * 8 + 1;
					int idx = SHIFTR(w0,  0,  8);
					switch (idx)
					{
						case G_MV_VIEWPORT:
							//typeName = "Viewport";
							break;
						case G_MV_LIGHT:
							//typeName = "Light";
							break;
						case G_MV_MATRIX:
							//typeName = "Forced Matrix";
							break;
						case G_MV_MMTX:
						case G_MV_PMTX:
						case G_MV_POINT:
						default:
							fprintf(stderr, "Unrecognized Movemem Index %d for data at %08X\n", idx, segAddr);
							break;
					}
					DataBlobSegmentPush(realAddr, len, w1, DATA_BLOB_TYPE_MATRIX);
					//ret = DisplayList_CopyMovemem(obj1, w1, , , obj2, &w1);
					//if (ret != 0)
					//	goto err;
				}
				break;
			
			case G_MTX:
				if ((realAddr = DataBlobSegmentAddressToRealAddress(w1))) {
					DataBlobSegmentPush(realAddr, SIZEOF_MTX, w1, DATA_BLOB_TYPE_MATRIX);
					//ret = DisplayList_CopyMtx(obj1, w1, obj2, &w1);
					//if (ret != 0)
					//	goto err;
				}
				break;
			
			case G_VTX:
				if ((realAddr = DataBlobSegmentAddressToRealAddress(w1))) {
					DataBlobSegmentPush(realAddr, (SHIFTR(w0, 12, 8)) * SIZEOF_VTX, w1, DATA_BLOB_TYPE_VERTEX);
					//ret = DisplayList_CopyVtx(obj1, w1, SHIFTR(w0, 12, 8), obj2, &w1);
					//if (ret != 0)
					//	goto err;
				}
				break;
			
			/*
			 * Texture and TLUT Loading
			 */
			
			case G_SETTIMG:
				timg.fmt =   SHIFTR(w0, 21, 3);
				timg.siz =   SHIFTR(w0, 19, 2);
				timg.width = SHIFTR(w0, 0, 12) + 1;
				timg.dram =  w1;
				break;
			
			case G_SETTILE:
				{
					int tile = SHIFTR(w1, 24, 3);
					
					tileDescriptors[tile].fmt =   SHIFTR(w0, 21, 3);
					tileDescriptors[tile].siz =   SHIFTR(w0, 19, 2);
					tileDescriptors[tile].line =  SHIFTR(w0,  9, 9);
					tileDescriptors[tile].tmem =  SHIFTR(w0,  0, 9);
					tileDescriptors[tile].pal =   SHIFTR(w1, 20, 4);
					tileDescriptors[tile].cms =   SHIFTR(w1,  8, 2);
					tileDescriptors[tile].cmt =   SHIFTR(w1, 18, 2);
					tileDescriptors[tile].masks =  SHIFTR(w1,  4, 4);
					tileDescriptors[tile].maskt =  SHIFTR(w1, 14, 4);
					tileDescriptors[tile].shifts = SHIFTR(w1,  0, 4);
					tileDescriptors[tile].shiftt = SHIFTR(w1, 10, 4);
				}
				break;
			
			case G_LOADTILE:
			case G_LOADBLOCK:
				// These commands are only used inside larger texture macros, so we don't need to do anything special with them
				break;
			
			case G_SETTILESIZE:
				{
					int tile = SHIFTR(w1, 24, 3);
					
					tileDescriptors[tile].uls =  SHIFTR(w0, 12, 12);
					tileDescriptors[tile].ult =  SHIFTR(w0,  0, 12);
					tileDescriptors[tile].lrs =  SHIFTR(w1, 12, 12);
					tileDescriptors[tile].lrt =  SHIFTR(w1,  0, 12);
				}
				
				if (HISTORY_GET(0) == G_SETTILE &&
					HISTORY_GET(1) == G_RDPPIPESYNC &&
					HISTORY_GET(2) == G_LOADBLOCK &&
					HISTORY_GET(3) == G_RDPLOADSYNC &&
					HISTORY_GET(4) == G_SETTILE &&
					HISTORY_GET(5) == G_SETTIMG)
				{
					int tile = SHIFTR(w1, 24, 3);
					// gsDPLoadTextureBlock / gsDPLoadMultiBlock
					uint32_t addr = timg.dram;
					int siz = tileDescriptors[tile].siz;
					uint32_t width = qu102_I(tileDescriptors[tile].lrs) + 1;
					uint32_t height = qu102_I(tileDescriptors[tile].lrt) + 1;
					
					if ((realAddr = DataBlobSegmentAddressToRealAddress(addr)))
					{
						size_t size = G_SIZ_BYTES(siz) * width * height;
						
						DataBlobSegmentPush(realAddr, size, addr, DATA_BLOB_TYPE_TEXTURE);
						//ret = DisplayList_CopyData(obj1, addr, size, obj2, &newAddr, "Texture/Multi Block");
						//if (ret != 0)
						//	goto err;
					}
				}
				break;
			
			case G_LOADTLUT:
				if (HISTORY_GET(0) == G_RDPLOADSYNC &&
					HISTORY_GET(1) == G_SETTILE &&
					HISTORY_GET(2) == G_RDPTILESYNC &&
					HISTORY_GET(3) == G_SETTIMG &&
				   (timg.fmt == G_IM_FMT_RGBA || timg.fmt == G_IM_FMT_IA) &&
					timg.siz == G_IM_SIZ_16b)
				{
					// gsDPLoadTLUT / gsDPLoadTLUT_pal16 / gsDPLoadTLUT_pal256
					uint32_t addr = timg.dram;
					uint32_t count = SHIFTR(w1, 14, 10) + 1;
					
					if ((realAddr = DataBlobSegmentAddressToRealAddress(addr)))
					{
						size_t size = ALIGN8(G_SIZ_BYTES(G_IM_SIZ_16b) * count);
						
						DataBlobSegmentPush(realAddr, size, addr, DATA_BLOB_TYPE_PALETTE);
						//ret = DisplayList_CopyData(obj1, addr, size, obj2, &newAddr, "TLUT");
						//if (ret != 0)
						//	goto err;
					}
				}
				break;
			
			/*
			 * These commands are 128 bits rather than the usual 64 bits
			 */
			
			case G_TEXRECTFLIP:
			case G_TEXRECT:
				cmdlen = 16;
				break;
			
			/*
			 * These should not appear in objects
			 */
			
			case G_MOVEWORD:
			case G_DMA_IO:
			case G_LOAD_UCODE:
			case G_SETCIMG:
			case G_SETZIMG:
				fprintf(stderr, "Unimplemented display list command %02X encountered in %08X\n", cmd, segAddr);
				break;
				//ret = DisplayList_ErrMsgSet("Unimplemented display list command %02X encountered in %08X\n", cmd, segAddr);
				//goto err;
			
			/*
			 * All other commands do not need any special handling, we know they are valid from the length check
			 */
			
			default:
				break;
		}

		// Increment to next command
		data += cmdlen;

		// Update history ringbuffer
		history[i] = cmd;
		i = (i + 1) % ARRLEN(history);
	}
//err:
//	do{}while(0);
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
