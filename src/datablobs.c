//
// datablobs.c
//
// for storing and manipulating data blobs
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <stb_image.h>
#include <n64texconv.h>

#include "logging.h"
#include "datablobs.h"
#include "stretchy_buffer.h"

#define F3DEX_GBI_2
#include "gbi.h"

void Die(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));

#include "misc.h" // for RoomShapeImage

// gbi extras
/* dl push flag */
#define G_DL_PUSH   0
#define G_DL_NOPUSH 1
/* 10.2 fixed point */
typedef uint16_t qu102_t;
#define qu102_I(x) \
    ((signed int)((x) >> 2))
#define G_SIZ_BYTES(siz) (G_SIZ_BITS(siz) / 8)
#define ALIGN8(x) (((x) + 7) & ~7)

#define READ_32_BE(BYTES, OFFSET) ( \
	(((const uint8_t*)(BYTES))[OFFSET + 0] << 24) | \
	(((const uint8_t*)(BYTES))[OFFSET + 1] << 16) | \
	(((const uint8_t*)(BYTES))[OFFSET + 2] <<  8) | \
	(((const uint8_t*)(BYTES))[OFFSET + 3] <<  0) \
)

#define PUT_32_BE(BYTES, OFFSET, DATA) { \
	((uint8_t*)(BYTES))[OFFSET + 0] = ((DATA) >> 24) & 0xff; \
	((uint8_t*)(BYTES))[OFFSET + 1] = ((DATA) >> 16) & 0xff; \
	((uint8_t*)(BYTES))[OFFSET + 2] = ((DATA) >>  8) & 0xff; \
	((uint8_t*)(BYTES))[OFFSET + 3] = ((DATA) >>  0) & 0xff; \
}

#define ARRLEN(X) (sizeof(X) / sizeof(*(X)))
#define FALLTHROUGH __attribute__((fallthrough))
#define SIZEOF_GFX 8
#define SIZEOF_MTX 0x40
#define SIZEOF_VTX 0x10
#define SHIFTL(v, s, w) \
	((uint32_t)(((uint32_t)(v) & ((0x01 << (w)) - 1)) << (s)))
#define SHIFTR(v, s, w) \
	((uint32_t)(((uint32_t)(v) >> (s)) & ((0x01 << (w)) - 1)))
#define ShiftR SHIFTR

static struct DataBlobSegment gSegments[16];

struct DataBlob *DataBlobNew(
	const void *refData
	, uint32_t sizeBytes
	, uint32_t segmentAddr
	, enum DataBlobType type
	, struct DataBlob *next
	, void *ref
)
{
	struct DataBlob *blob = calloc(1, sizeof(*blob));
	
	*blob = (struct DataBlob) {
		.next = next
		, .refData = refData
		, .originalSegmentAddress = segmentAddr
		, .sizeBytes = sizeBytes
		, .type = type
		, .alignBytes = 8
	};
	
	if (ref)
		sb_push(blob->refs, ref);
	
	return blob;
}

void DatablobFree(struct DataBlob *blob)
{
	sb_free(blob->refs);
	
	sb_free(blob->callbacks.postsort);
	
	if (blob->ownsRefData)
		free((void*)blob->refData);
	
	free(blob);
}

void DatablobFreeList(struct DataBlob *listHead)
{
	struct DataBlob *next = 0;
	
	for ( ; listHead; listHead = next)
	{
		next = listHead->next;
		
		DatablobFree(listHead);
	}
}

void DataBlobRemoveRef(struct DataBlob *blob, void *ref)
{
	if (!blob)
		return;
	
	sb_foreach_backwards(blob->refs, {
		if (*each == ref)
			sb_remove(blob->refs, eachIndex);
	})
	
	LogDebug("removed references to %08x, leaves %d", blob->originalSegmentAddress, sb_count(blob->refs));
}

void DataBlobListRemoveBlankEntries(struct DataBlob **listHead)
{
	struct DataBlob *prev = 0;
	struct DataBlob *next = 0;
	
	for (struct DataBlob *blob = *listHead; blob; blob = next)
	{
		next = blob->next;
		
		// unlink from list
		if (blob->refData == 0)
		{
			if (prev == 0)
				*listHead = next;
			else
				prev->next = next;
			
			DatablobFree(blob);
			continue;
		}
		
		prev = blob;
	}
}

uint8_t *DataBlobToTruecolor(struct DataBlob *blob, int *width, int *height, uint8_t *optionalDst)
{
	static uint8_t *imageData = 0;
	struct DataBlob *palBlob = blob->data.texture.pal;
	int imageWidth = blob->data.texture.w;
	int imageHeight = blob->data.texture.h;
	int imageFmt = blob->data.texture.fmt;
	int imageSiz = blob->data.texture.siz;
	//uint32_t segAddr = blob->originalSegmentAddress;
	//uint32_t palAddr = palBlob ? palBlob->originalSegmentAddress : 0;
	int sizeBytesClamped = blob->data.texture.sizeBytesClamped;
	uint8_t *dst;
	
	if (width) *width = imageWidth;
	if (height) *height = imageHeight;
	
	// n64 tmem = 4kib, *4 for 32-bit color conversion
	// and *2 b/c 4bit textures expand to *2*4x the bytes
	if (!imageData)
		imageData = (uint8_t*)calloc(4, 512 * 512); // for prerenders
		//imageData = (uint8_t*)malloc(4096 * 4 * 2);
	
	dst = optionalDst ? optionalDst : imageData;
	
	if (sizeBytesClamped > 4096
		|| (((uint8_t*)blob->refData) + sizeBytesClamped) > (uint8_t*)blob->refDataFileEnd
		// bounds checking for palette, if applicable:
		|| (palBlob && (((uint8_t*)palBlob->refData) + palBlob->sizeBytes) > (uint8_t*)palBlob->refDataFileEnd)
	)
	{
		LogDebug("warning: width height %d x %d", imageWidth, imageHeight);
		LogDebug("refData    = %p", blob->refData);
		LogDebug("refDataEnd = %p", blob->refDataFileEnd);
		LogDebug("sizeBytes  = %x", blob->sizeBytes);
		return 0;
	}
	
	if (blob->data.texture.isJfif)
	{
		int x, y, c;
		void *test = stbi_load_from_memory(
			(const stbi_uc*)blob->refData
			, blob->sizeBytes
			, &x, &y, &c, 4
		);
		//LogDebug("test = %p %d x %d x %d", test, x, y, c);
		memcpy(dst, test, x * y * 4);
		stbi_image_free(test);
	}
	else
	{
		n64texconv_to_rgba8888(
			dst
			, (unsigned char*)blob->refData // TODO const correctness
			, (unsigned char*)(palBlob ? palBlob->refData : 0)
			, (enum n64texconv_fmt)imageFmt
			, (enum n64texconv_bpp)imageSiz
			, imageWidth
			, imageHeight
			, blob->data.texture.lineSize
		);
	}
	
	return dst;
}

// generates output intended for use as z64texture input
// TODO some quality-of-life stuff like optimizing duplicate textures, grouping textures by palette, etc
void DataBlobListMakeTextureBank(struct DataBlob *listHead, FILE *file, FILE *recipe, char *texpath)
{
	static int palsWritten = 0;
	
	// reinitialization call
	if (listHead == 0)
	{
		palsWritten = 0;
		return;
	}
	
	char *name = texpath + strlen(texpath); // points to 'w' in 'textures/w'
	
	// write palettes first
	for (struct DataBlob *blob = listHead; blob; blob = blob->next)
	{
		if (blob->type == DATA_BLOB_TYPE_PALETTE)
		{
			int size = blob->sizeBytes; // palettes have to use sizeBytes
			//fprintf(recipe, "write palette 0x%x bytes\n", size);
			unsigned int offset = ftell(file);
			fprintf(recipe, "%d,pal-%d,rgba16,auto,0x%x\n", size / 2, palsWritten, offset);
			blob->updatedSegmentAddress = offset | 0x06000000;
			blob->originalSegmentAddress = blob->updatedSegmentAddress;
			fwrite(blob->refData, 1, size, file);
			blob->sizeBytes = 0;
			
			blob->data.pal.id = palsWritten;
			palsWritten += 1;
		}
	}
	
	// followed by textures
	for (struct DataBlob *blob = listHead; blob; blob = blob->next)
	{
		if (blob->type == DATA_BLOB_TYPE_TEXTURE)
		{
			const char *fmt[] = { "rgba", "yuv", "ci", "ia", "i" };
			const char *bpp[] = { "4", "8", "16", "32" };
			struct DataBlob *pal = blob->data.texture.pal;
			int size = blob->data.texture.sizeBytesClamped; // or use blob->sizeBytes
			int w = blob->data.texture.w;
			int h = blob->data.texture.h;
			unsigned int offset = ftell(file);
			char fmtStr[32];
			
			sprintf(fmtStr, "%s%s", fmt[blob->data.texture.fmt], bpp[blob->data.texture.siz]);
			sprintf(name, "%08x.png", offset | 0x06000000);
			
			if (pal)
				fprintf(recipe, "%dx%d,%s-%d,%s,0x%x\n",
					w, h, fmtStr, pal->data.pal.id, name, offset
				);
			else
				fprintf(recipe, "%dx%d,%s,%s,0x%x\n",
					w, h, fmtStr, name, offset
				);
			blob->updatedSegmentAddress = offset | 0x06000000;
			blob->originalSegmentAddress = blob->updatedSegmentAddress;
			fwrite(blob->refData, 1, size, file);
			blob->sizeBytes = 0;
		}
	}
	
	*name = '\0'; // revert filename to dirname
}

struct DataBlob *DataBlobListFindBlobWithOriginalSegmentAddress(
	struct DataBlob *listHead
	, uint32_t originalSegmentAddress
)
{
	for (struct DataBlob *blob = listHead; blob; blob = blob->next)
		if (blob->originalSegmentAddress == originalSegmentAddress)
			return blob;
	
	return 0;
}

// listHead = DataBlobPush(listHead, ...)
struct DataBlob *DataBlobPush(
	struct DataBlob *listHead
	, const void *refData
	, uint32_t sizeBytes
	, uint32_t segmentAddr
	, enum DataBlobType type
	, void *ref
)
{
	struct DataBlob *prev = listHead;
	
	for (struct DataBlob *each = listHead; each; each = each->next)
	{
		// already in list
		if (each->originalSegmentAddress == segmentAddr)
		{
			// it's possible for blocks to be coalesced
			if (sizeBytes > each->sizeBytes)
				each->sizeBytes = sizeBytes;
			
			// keep track of references to ram segment
			if (ref && !sb_contains_ref(each->refs, ref))
				sb_push(each->refs, ref);
			
			// already at beginning of list
			if (each == listHead)
				return listHead;
			
			// move to beginning of list
			if (prev)
				prev->next = each->next;
			each->next = listHead;
			return each;
		}
		
		prev = each;
	}
	
	// prepend so addresses can later be resolved in reverse order
	return DataBlobNew(refData, sizeBytes, segmentAddr, type, listHead, ref);
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

// promote blob to beginning of list
void DataBlobListTouchBlob(struct DataBlob **listHead, struct DataBlob *blob)
{
	struct DataBlob *prev = *listHead;
	
	// already at beginning of list
	if (blob == *listHead)
		return;
	
	// find in list
	for (struct DataBlob *each = *listHead; each; each = each->next)
	{
		// move to beginning of list
		if (each == blob)
		{
			if (prev)
				prev->next = each->next;
			
			each->next = *listHead;
			*listHead = each;
			
			return;
		}
		
		prev = each;
	}
}

void DataBlobSegmentClearAll(void)
{
	memset(gSegments, 0, sizeof(gSegments));
}

void DataBlobSegmentSetup(int segmentIndex, const void *data, const void *dataEnd, struct DataBlob *head)
{
	struct DataBlobSegment *seg = &gSegments[segmentIndex];
	
	LogDebug("DataBlobSegmentSetup(%d): %p - %p (%08x)"
		, segmentIndex, data, dataEnd, (uint32_t)(dataEnd - data)
	);
	
	seg->head = head;
	seg->data = data;
	seg->dataEnd = dataEnd;
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

bool DataBlobSegmentContainsSegAddr(int segmentIndex, uint32_t originalSegmentAddress)
{
	struct DataBlob *blobs = DataBlobSegmentGetHead(segmentIndex);
	
	if (blobs)
		datablob_foreach(blobs, {
			if (each->originalSegmentAddress == originalSegmentAddress)
				return true;
		})
	
	return false;
}

struct DataBlob *DataBlobSegmentPush(
	const void *refData
	, uint32_t sizeBytes
	, uint32_t segmentAddr
	, enum DataBlobType type
	, void *ref
)
{
	struct DataBlobSegment *seg = DataBlobSegmentGet(segmentAddr >> 24);
	
	if (!seg)
		return 0;
	
	seg->head = DataBlobPush(seg->head, refData, sizeBytes, segmentAddr, type, ref);
	seg->head->refDataFileEnd = seg->dataEnd;
	
	return seg->head;
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

const void *DataBlobSegmentAddressBoundsEnd(uint32_t segAddr)
{
	// skip unpopulated segments
	if ((segAddr >> 24) >= ARRLEN(gSegments)
		|| gSegments[segAddr >> 24].data == 0
	)
		return 0;
	
	struct DataBlobSegment *seg = DataBlobSegmentGet(segAddr >> 24);
	
	return seg->dataEnd;
}

// adapted from DisplayList_Copy() from Tharo's dlcopy for this
// https://github.com/Thar0/dlcopy/
// DisplayList_Copy (ZObj* obj1, segaddr_t segAddr, ZObj* obj2, segaddr_t* newSegAddr)
void DataBlobSegmentsPopulateFromMesh(uint32_t segAddr, void *originator)
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
		void *dramRef;
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
	
	static struct DataBlob *palBlob = 0;
	static sb_array(struct DataBlob *, needsPalettes); // color-indexed textures w/o palettes
	
	bool exit = false;
	for (const uint8_t *data = DataBlobSegmentAddressToRealAddress(segAddr); !exit; )
	{
		size_t cmdlen = SIZEOF_GFX;
		uint32_t w0 = READ_32_BE(data, 0);
		uint32_t w1 = READ_32_BE(data, 4);
		void *w1addr = (void*)(data + 4);
		
		int cmd = w0 >> 24;
		switch (cmd)
		{
			/*
			 * These commands contain pointers, except G_ENDDL which is here for convenience
			 */
			
			case G_DL:
				// recursively copy called display lists
				DataBlobSegmentsPopulateFromMesh(w1, w1addr);
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
							LogDebug("Unrecognized Movemem Index %d for data at %08X", idx, segAddr);
							break;
					}
					DataBlobSegmentPush(realAddr, len, w1, DATA_BLOB_TYPE_GENERIC, w1addr);
					//ret = DisplayList_CopyMovemem(obj1, w1, , , obj2, &w1);
					//if (ret != 0)
					//	goto err;
				}
				break;
			
			case G_MTX:
				if ((realAddr = DataBlobSegmentAddressToRealAddress(w1))) {
					DataBlobSegmentPush(realAddr, SIZEOF_MTX, w1, DATA_BLOB_TYPE_MATRIX, w1addr);
					//ret = DisplayList_CopyMtx(obj1, w1, obj2, &w1);
					//if (ret != 0)
					//	goto err;
				}
				break;
			
			case G_VTX:
				if ((realAddr = DataBlobSegmentAddressToRealAddress(w1))) {
					DataBlobSegmentPush(realAddr, (SHIFTR(w0, 12, 8)) * SIZEOF_VTX, w1, DATA_BLOB_TYPE_VERTEX, w1addr);
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
				timg.dramRef = w1addr;
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
						struct DataBlob *blob;
						
						// old method was returning 0 here
						if (siz == G_IM_SIZ_4b)
							size = (width * height) / 2;
						else
							size = G_SIZ_BYTES(siz) * width * height;
						
						if (size > 4096)
							LogDebug("warning: width height %d x %d", width, height);
						
						blob = DataBlobSegmentPush(realAddr, size, addr, DATA_BLOB_TYPE_TEXTURE, timg.dramRef);
						blob->data.texture.w = width;
						blob->data.texture.h = height;
						blob->data.texture.siz = siz;
						blob->data.texture.fmt = tileDescriptors[tile].fmt;
						if (blob->data.texture.fmt == G_IM_FMT_CI
							&& blob->data.texture.pal == 0
						)
							sb_push(needsPalettes, blob);
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
						
						palBlob = DataBlobSegmentPush(realAddr, size, addr, DATA_BLOB_TYPE_PALETTE, timg.dramRef);
						//ret = DisplayList_CopyData(obj1, addr, size, obj2, &newAddr, "TLUT");
						//if (ret != 0)
						//	goto err;
					}
				}
				break;
			
			// game should be ready to draw at this point, so use
			// this as an opportunity to resolve palette address
			case G_TRI1:
			case G_TRI2:
				if (sb_count(needsPalettes))
				{
					sb_foreach(needsPalettes, {
						(*each)->data.texture.pal = palBlob;
					});
					
					sb_clear(needsPalettes);
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
				LogDebug("Unimplemented display list command %02X encountered in %08X", cmd, segAddr);
				break;
				//ret = DisplayList_ErrMsgSet("Unimplemented display list command %02X encountered in %08X", cmd, segAddr);
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

// new method, based on cooliscool's uot: https://code.google.com/p/uot
// TODO nested functions are not standard c99, so refactor this eventually
void DataBlobSegmentsPopulateFromMeshNew(uint32_t segAddr, void *originator)
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
	
	realAddr = DataBlobSegmentAddressToRealAddress(segAddr);
	
	if (!realAddr)
		return;
	
	struct DataBlob *dlBlob = DataBlobSegmentPush(realAddr, 0, segAddr, DATA_BLOB_TYPE_MESH, originator);
	
	// XXX
	// process it again, so that DL's ref'd by these DL's ref'd by multiple
	// room mesh headers also get moved to the list head again and are thus
	// refreshed/prioritized (so most deeply nested DL's are updated first)
	// (leaving original code commented out below as a reminder not to do this)
	// already processed
	//if (dlBlob->sizeBytes)
	//	return;
	
	// don't cumulatively increase size if it's encountered multiple times
	// (just reset size to 0 and restart the calculation each time)
	dlBlob->sizeBytes = 0;
	
#define HISTORY_GET(n) \
	history[(i - 1 - (n) < 0) ? (i - 1 - (n) + ARRLEN(history)) : (i - 1 - (n))]
	
	static struct {
		uint32_t Dram;
		void *DramRef;
		int Width;
		int Height;
		int RealWidth;
		int RealHeight;
		float ShiftS;
		float ShiftT;
		float S_Scale;
		float T_Scale;
		float TextureHRatio;
		float TextureWRatio;
		// SetTile
		int TexFormat;
		int TexFormatUOT;
		int TexelSize;
		int LineSize;
		int CMT;
		int CMS;
		int MaskS;
		int MaskT;
		int TShiftS;
		int TShiftT;
		// SetTileSize
		qu102_t ULS;
		qu102_t ULT;
		qu102_t LRS;
		qu102_t LRT;
	} textures[2];
	#define Textures(X) textures[X]
	static int CurrentTex;
	static bool MultiTexCoord; (void)MultiTexCoord;
	static bool MultiTexture; (void)MultiTexture;
	static uint32_t gRdpHalf1 = 0;
	static void *gRdpHalf1w1addr = 0;
	
	static struct DataBlob *palBlob = 0;
	static sb_array(struct DataBlob *, needsPalettes); // color-indexed textures w/o palettes
	
	int Pow2(int val)
	{
		int i = 1;
		while (i < val)
			i <<= 1;
		return i;
	}
	int PowOf(int val)
	{
		int num = 1;
		int i = 0;
		while (num < val)
		{
			num <<= 1;
			i += 1;
		}
		return i;
	}
	float Fixed2Float(float v, int b)
	{
		float FIXED2FLOATRECIP[] = { 0.5F, 0.25F, 0.125F, 0.0625F, 0.03125F
			, 0.015625F, 0.0078125F, 0.00390625F, 0.001953125F, 0.0009765625F
			, 0.00048828125F, 0.000244140625F, 0.000122070313F, 0.0000610351563F
			, 0.0000305175781F, 0.0000152587891F
		};
		return v * FIXED2FLOATRECIP[b - 1];
	}
	void CalculateTexSize(int id)
	{
		int MaxTexel = 0;
		int Line_Shift = 0;
		
		switch (Textures(id).TexFormatUOT)
		{
			case 0x00: case 0x40:
				MaxTexel = 4096;
				Line_Shift = 4;
				break;
			case 0x60: case 0x80:
				MaxTexel = 8192;
				Line_Shift = 4;
				break;
			case 0x8: case 0x48:
				MaxTexel = 2048;
				Line_Shift = 3;
				break;
			case 0x68: case 0x88:
				MaxTexel = 4096;
				Line_Shift = 3;
				break;
			case 0x10: case 0x70:
				MaxTexel = 2048;
				Line_Shift = 2;
				break;
			case 0x50: case 0x90:
				MaxTexel = 2048;
				Line_Shift = 0;
				break;
			case 0x18:
				MaxTexel = 1024;
				Line_Shift = 2;
				break;
		}
		
		int Line_Width = Textures(id).LineSize << Line_Shift;
		
		int Tile_Width = Textures(id).LRS - Textures(id).ULS + 1;
		int Tile_Height = Textures(id).LRT - Textures(id).ULT + 1;
		
		int Mask_Width = 1 << Textures(id).MaskS;
		int Mask_Height = 1 << Textures(id).MaskT;
		
		int Line_Height = 0;
		if (Line_Width > 0)
			Line_Height = Min(MaxTexel / Line_Width, Tile_Height);
		
		// NPOT
		if (true)
		{
			if (Textures(id).MaskS > 0 && ((Mask_Width * Mask_Height) <= MaxTexel))
				Textures(id).RealWidth = Mask_Width;
			else if ((Tile_Width * Tile_Height) <= MaxTexel)
				Textures(id).RealWidth = Tile_Width;
			else
				Textures(id).RealWidth = (Textures(id).Width >> 2) + 1;//Line_Width;
			
			if (Textures(id).MaskT > 0 && ((Mask_Width * Mask_Height) <= MaxTexel))
				Textures(id).RealHeight = Mask_Height;
			else if ((Tile_Width * Tile_Height) <= MaxTexel)
				Textures(id).RealHeight = Tile_Height;
			else
				Textures(id).RealHeight = Min(Line_Height, (Textures(id).Height >> 2) + 1);//Line_Height;
			
			#ifndef NDEBUG
			{
				int width = Textures(id).RealWidth;
				int height = Textures(id).RealHeight;
				int siz = Textures(CurrentTex).TexelSize;
				size_t size = 0;
				
				// old method was returning 0 here
				if (siz == G_IM_SIZ_4b) size = (width * height) / 2;
				else size = G_SIZ_BYTES(siz) * width * height;
				
				if (size > 4096) Die("this shouldn't happen");
			}
			#endif
			
			return;
		}
		// testing NPOT
		/*
		if (false)
		{
			// fixes NPOT textures, is it this simple?
			//uint32_t width = fabs(qu102_I(Textures(id).ULS) - qu102_I(Textures(id).LRS)) + 1;
			//uint32_t height = fabs(qu102_I(Textures(id).ULT) - qu102_I(Textures(id).LRT)) + 1;
			uint32_t width = qu102_I(Textures(id).LRS) + 1;
			uint32_t height = qu102_I(Textures(id).LRT) + 1;
			//LogDebug("%d %d %d %d", Line_Width, Line_Height, width, height);
			//LogDebug(" -> %d %d", Textures(id).LRS, Textures(id).LRT);
			//LogDebug(" -> %d %d", Textures(id).ULS, Textures(id).ULT);
			
			//int scaleDown = Textures(id).LineSize / 4;
			//if (!scaleDown) scaleDown = 1;
			//width /= scaleDown;
			//height /= scaleDown;
			
			Textures(id).RealWidth = width;
			Textures(id).RealHeight = height;
			
			LogDebug("%d %d - %d", width, height, Textures(id).LineSize);
			
			int siz = Textures(CurrentTex).TexelSize;
			size_t size = G_SIZ_BYTES(siz) * width * height;
			
			// old method was returning 0 here
			if (siz == G_IM_SIZ_4b)
				size = (width * height) / 2;
			else
				size = G_SIZ_BYTES(siz) * width * height;
			
			// maybe?
			if (size > 4096)
			{
				LogDebug("mask s t %d %d", Textures(id).MaskS, Textures(id).MaskT);
				bool axis = 0;
				while (size > 4096)
				{
					if (!axis) height /= 2;
					else width /= 2;
					
					// old method was returning 0 here
					if (siz == G_IM_SIZ_4b) size = (width * height) / 2;
					else size = G_SIZ_BYTES(siz) * width * height;
				}
				
				Textures(id).RealWidth = width;
				Textures(id).RealHeight = height;
			}
			
			if (size <= 4096)
				return;
			
			Die("this shouldn't happen");
		}
		*/
		
		if (Textures(id).MaskS > 0 && ((Mask_Width * Mask_Height) <= MaxTexel))
			Textures(id).Width = Mask_Width;
		else if ((Tile_Width * Tile_Height) <= MaxTexel)
			Textures(id).Width = Tile_Width;
		else
			Textures(id).Width = Line_Width;
		
		if (Textures(id).MaskT > 0 && ((Mask_Width * Mask_Height) <= MaxTexel))
			Textures(id).Height = Mask_Height;
		else if ((Tile_Width * Tile_Height) <= MaxTexel)
			Textures(id).Height = Tile_Height;
		else
			Textures(id).Height = Line_Height;
		
		int Clamp_Width = 0;
		int Clamp_Height = 0;
		if (Textures(id).CMS == 1)
			Clamp_Width = Tile_Width;
		else
			Clamp_Width = Textures(id).Width;
		if (Textures(id).CMT == 1)
			Clamp_Height = Tile_Height;
		else
			Clamp_Height = Textures(id).Height;
		
		if (Mask_Width > Textures(id).Width)
		{
			Textures(id).MaskS = PowOf(Textures(id).Width);
			Mask_Width = 1 << Textures(id).MaskS;
		}
		if (Mask_Height > Textures(id).Height)
		{
			Textures(id).MaskT = PowOf(Textures(id).Height);
			Mask_Height = 1 << Textures(id).MaskT;
		}
		
		if (Textures(id).CMS == 2 || Textures(id).CMS == 3)
			Textures(id).RealWidth = Pow2(Clamp_Width);
		else if (Textures(id).CMS == 1)
			Textures(id).RealWidth = Pow2(Mask_Width);
		else
			Textures(id).RealWidth = Pow2(Textures(id).Width);
		
		if (Textures(id).CMT == 2 || Textures(id).CMT == 3)
			Textures(id).RealHeight = Pow2(Clamp_Height);
		else if (Textures(id).CMT == 1)
			Textures(id).RealHeight = Pow2(Mask_Height);
		else
			Textures(id).RealHeight = Pow2(Textures(id).Height);
		
		Textures(id).ShiftS = 1.0f;
		Textures(id).ShiftT = 1.0f;
		
		if (Textures(id).TShiftS > 10)
			Textures(id).ShiftS = (1 << (16 - Textures(id).TShiftS));
		else if (Textures(id).TShiftS > 0)
			Textures(id).ShiftS /= (1 << Textures(id).TShiftS);
		
		if (Textures(id).TShiftT > 10)
			Textures(id).ShiftT = (1 << (16 - Textures(id).TShiftT));
		else if (Textures(id).TShiftT > 0)
			Textures(id).ShiftT /= (1 << Textures(id).TShiftT);
		
		Textures(id).TextureHRatio = ((Textures(id).T_Scale * Textures(id).ShiftT) / 32.0f / Textures(id).RealHeight);
		Textures(id).TextureWRatio = ((Textures(id).S_Scale * Textures(id).ShiftS) / 32.0f / Textures(id).RealWidth);
	}
	
	bool exit = false;
	for (const uint8_t *data = dlBlob->refData; !exit; dlBlob->sizeBytes += SIZEOF_GFX, data += SIZEOF_GFX)
	{
		uint32_t w0 = READ_32_BE(data, 0);
		uint32_t w1 = READ_32_BE(data, 4);
		void *w1addr = (void*)(data + 4);
		
		int cmd = w0 >> 24;
		switch (cmd)
		{
			/*
			 * These commands contain pointers, except G_ENDDL which is here for convenience
			 */
			
			case G_DL:
				// recursively copy called display lists
				DataBlobSegmentsPopulateFromMeshNew(w1, w1addr);
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
							LogDebug("Unrecognized Movemem Index %d for data at %08X", idx, segAddr);
							break;
					}
					DataBlobSegmentPush(realAddr, len, w1, DATA_BLOB_TYPE_GENERIC, w1addr);
				}
				break;
			
			case G_MTX:
				if ((realAddr = DataBlobSegmentAddressToRealAddress(w1)))
					DataBlobSegmentPush(realAddr, SIZEOF_MTX, w1, DATA_BLOB_TYPE_MATRIX, w1addr);
				break;
			
			case G_VTX:
				if ((realAddr = DataBlobSegmentAddressToRealAddress(w1)))
					DataBlobSegmentPush(realAddr, (SHIFTR(w0, 12, 8)) * SIZEOF_VTX, w1, DATA_BLOB_TYPE_VERTEX, w1addr);
				break;
			
			/*
			 * Texture and TLUT Loading
			 */
			
			case G_SETTIMG: {
				bool palMode = data[8] == G_RDPTILESYNC;
				if (HISTORY_GET(0) == G_SETTILESIZE)
				{
					CurrentTex = 1;
					if (true) // GLExtensions.GLMultiTexture And GLExtensions.GLFragProg
						MultiTexCoord = true;
					else if (false)
						MultiTexCoord = false;
					MultiTexture = true;
				}
				else
				{
					CurrentTex = 0;
					MultiTexCoord = false;
					MultiTexture = false;
				}
				if (palMode)
				{
					Textures(0).Dram = w1;
					Textures(0).DramRef = w1addr;
				}
				else
				{
					Textures(CurrentTex).Dram = w1;
					Textures(CurrentTex).DramRef = w1addr;
				}
				break;
			}
			
			case G_SETTILE:
				//If .CMDParams(1) > 0 Then SETTILE(.CMDLow, .CMDHigh)
				{
					//int tile = SHIFTR(w1, 24, 3);
					
					Textures(CurrentTex).TexFormatUOT = (w0 >> 16) & 0xff; // uot
					Textures(CurrentTex).TexFormat =   SHIFTR(w0, 21, 3);
					Textures(CurrentTex).TexelSize =   SHIFTR(w0, 19, 2);
					Textures(CurrentTex).LineSize =  SHIFTR(w0,  9, 9);
					//tileDescriptors[tile].tmem =  SHIFTR(w0,  0, 9); // unused
					//tileDescriptors[tile].pal =   SHIFTR(w1, 20, 4); // unused
					Textures(CurrentTex).CMT = ShiftR(w1, 18, 2);
					Textures(CurrentTex).CMS = ShiftR(w1, 8, 2);
					Textures(CurrentTex).MaskS = ShiftR(w1, 4, 4);
					Textures(CurrentTex).MaskT = ShiftR(w1, 14, 4);
					Textures(CurrentTex).TShiftS = ShiftR(w1, 0, 4);
					Textures(CurrentTex).TShiftT = ShiftR(w1, 10, 4);
					
					// getting linesize of 8 on a 32x32 rgba truecolor texture,
					// should be 16, so maybe this math is required?
					if (Textures(CurrentTex).TexelSize == G_IM_SIZ_32b)
						Textures(CurrentTex).LineSize *= 2;
				}
				break;
			
			case G_LOADTILE:
			case G_LOADBLOCK:
				// These commands are only used inside larger texture macros, so we don't need to do anything special with them
				break;
			
			// TODO support LOD textures
			case G_SETOTHERMODE_L:
			case G_SETOTHERMODE_H:
				break;
			
			case G_SETTILESIZE: {
				{
					Textures(CurrentTex).ULS =  SHIFTR(w0, 12, 12);
					Textures(CurrentTex).ULT =  SHIFTR(w0,  0, 12);
					Textures(CurrentTex).LRS =  SHIFTR(w1, 12, 12);
					Textures(CurrentTex).LRT =  SHIFTR(w1,  0, 12);
					Textures(CurrentTex).Width =  ((Textures(CurrentTex).LRS - Textures(CurrentTex).ULS) + 1);
					Textures(CurrentTex).Height = ((Textures(CurrentTex).LRT - Textures(CurrentTex).ULT) + 1);
				}
				
				CalculateTexSize(CurrentTex);
				
				// gsDPLoadTextureBlock / gsDPLoadMultiBlock
				uint32_t addr = Textures(CurrentTex).Dram;
				int siz = Textures(CurrentTex).TexelSize;
				uint32_t width = Textures(CurrentTex).RealWidth;
				uint32_t height = Textures(CurrentTex).RealHeight;
				
				// allows texture data blobs from unpopulated external segments, for flipbooks
				if ((realAddr = DataBlobSegmentAddressToRealAddress(addr)) || true)
				{
					int lineSize = Textures(CurrentTex).LineSize;
					size_t lineSizeBytes = lineSize * sizeof(uint64_t);
					int trueWidth = (Textures(CurrentTex).Width >> 2) + 1; // width of tex within file
					int trueHeight = (Textures(CurrentTex).Height >> 2) + 1; // height of tex within file
					
					if (trueWidth < 0) trueWidth = width;
					if (trueHeight < 0) trueHeight = height;
					
					// override for embedded textures with non-standard dimensions
					if (trueWidth % 4) {
						if (siz == G_IM_SIZ_4b)
							lineSizeBytes = trueWidth / 2;
						else
							lineSizeBytes = G_SIZ_BYTES(siz) * trueWidth;
						lineSize = 0;
					}
					
					size_t size = lineSizeBytes * trueHeight;
					size_t sizeBytesClamped = lineSizeBytes * height;
					struct DataBlob *blob;
					
					// G_SIZ_BYTES(G_IM_SIZ_4b) is 0, so handle separately
					/*
					if (siz == G_IM_SIZ_4b)
						size = (width * height) / 2;
					else
						size = G_SIZ_BYTES(siz) * width * height;
					*/
					
					if (size > 4096)
						LogDebug("warning: width height %d x %d", trueWidth, trueHeight);
					
					blob = DataBlobSegmentPush(realAddr, size, addr, DATA_BLOB_TYPE_TEXTURE, Textures(CurrentTex).DramRef);
					
					// extra safety
					if (!blob)
						break;
					
					// don't overwrite texture size if size already set
					if (!blob->data.texture.w)
					{
						blob->data.texture.w = width;
						blob->data.texture.h = height;
						blob->data.texture.siz = siz;
						blob->data.texture.fmt = Textures(CurrentTex).TexFormat;
						blob->data.texture.lineSize = lineSize;
						
						if (blob->data.texture.fmt == G_IM_FMT_CI
							&& blob->data.texture.pal == 0
						)
							sb_push(needsPalettes, blob);
					}
					if (sizeBytesClamped > blob->data.texture.sizeBytesClamped)
						blob->data.texture.sizeBytesClamped = sizeBytesClamped;
				}
				break;
			}
			
			case G_LOADTLUT: {
				{
					uint32_t addr = Textures(0).Dram;
					uint32_t count = SHIFTR(w1, 14, 10) + 1;
					
					if ((realAddr = DataBlobSegmentAddressToRealAddress(addr)))
					{
						size_t size = ALIGN8(G_SIZ_BYTES(G_IM_SIZ_16b) * count);
						
						palBlob = DataBlobSegmentPush(realAddr, size, addr, DATA_BLOB_TYPE_PALETTE, Textures(0).DramRef);
					}
				}
				break;
			}
			
			case G_TEXTURE: {
				uint32_t w1x = w1 & 0x00ffffff;
				for (int i = 0; i <= 1; ++i)
				{
					if (ShiftR(w1x, 16, 16) < 0xFFFF)
						Textures(i).S_Scale = Fixed2Float(ShiftR(w1x, 16, 16), 16);
					else
						Textures(i).S_Scale = 1.0F;
					
					if (ShiftR(w1x, 0, 16) < 0xFFFF)
						Textures(i).T_Scale = Fixed2Float(ShiftR(w1x, 0, 16), 16);
					else
						Textures(i).T_Scale = 1.0F;
				}
				break;
			}
			
			case G_RDPHALF_1:
				gRdpHalf1 = w1;
				gRdpHalf1w1addr = w1addr;
				break;
			
			case G_BRANCH_Z:
				DataBlobSegmentsPopulateFromMeshNew(gRdpHalf1, gRdpHalf1w1addr);
				break;
			
			// game should be ready to draw at this point, so use
			// this as an opportunity to resolve palette address
			case G_TRI1:
			case G_TRI2:
				if (sb_count(needsPalettes))
				{
					sb_foreach(needsPalettes, {
						(*each)->data.texture.pal = palBlob;
					});
					
					sb_clear(needsPalettes);
				}
				break;
			
			/*
			 * These should not appear in objects
			 */
			
			case G_MOVEWORD:
			case G_DMA_IO:
			case G_LOAD_UCODE:
			case G_SETCIMG:
			case G_SETZIMG:
				LogDebug("Unimplemented display list command %02X encountered in %08X", cmd, segAddr);
				break;
				//ret = DisplayList_ErrMsgSet("Unimplemented display list command %02X encountered in %08X", cmd, segAddr);
				//goto err;
			
			/*
			 * All other commands do not need any special handling, we know they are valid from the length check
			 */
			
			default:
				break;
		}
		
		// Update history ringbuffer
		history[i] = cmd;
		i = (i + 1) % ARRLEN(history);
	}
}

void DataBlobSegmentsPopulateFromRoomShapeImage(void *roomShapeImage)
{
	RoomShapeImage *image = roomShapeImage;
	struct DataBlob *blob;
	int siz = image->siz;
	int fmt = image->fmt;
	int width = image->width;
	int height = image->height;
	size_t size = G_SIZ_BYTES(siz) * width * height;
	uint32_t segAddr = image->source;
	const void *thisData = DataBlobSegmentAddressToRealAddress(segAddr);
	
	if (!segAddr)
		return;
	
	if (siz == G_IM_SIZ_4b) size = (width * height) / 2;
	else size = G_SIZ_BYTES(siz) * width * height;
	
	if (true) // assume always jfif
	{
		//
		// assuming each one is this big is fine, because
		// the texture blobs get sorted and trimmed at a
		// later step anyway
		//
		// on a somewhat related note, jfif data blobs
		// embed a block of blank data immediately after
		// themselves so the game doesn't have to allocate
		// a new buffer for decoding; this is accounted for
		// even with the trimming
		//
		size = ((const uint8_t*)DataBlobSegmentGet(segAddr >> 24)->dataEnd)
			-  ((const uint8_t*)thisData)
		;
	}
	
	// texture
	blob = DataBlobSegmentPush(
		thisData
		, size
		, segAddr
		, DATA_BLOB_TYPE_TEXTURE
		, &image->sourceBEU32
	);
	
	// don't overwrite texture size if size already set
	if (!blob->data.texture.w)
	{
		blob->data.texture.w = width;
		blob->data.texture.h = height;
		blob->data.texture.siz = siz;
		blob->data.texture.fmt = fmt;
		blob->data.texture.lineSize = 0;//lineSize; // TODO
		blob->data.texture.isJfif = true; // TODO assuming always jfif
	}
	
	// palette
	if (image->tlut)
		DataBlobSegmentPush(
			DataBlobSegmentAddressToRealAddress(image->tlut)
			, 0x200 // TODO consider image->tlutCount
			, image->tlut
			, DATA_BLOB_TYPE_PALETTE
			, &image->tlutBEU32
		);
}

void DataBlobApplyUpdatedSegmentAddresses(struct DataBlob *blob)
{
	if (!blob->refs) LogDebug("warning: no refs on %08x", blob->originalSegmentAddress);
	
	/*
	if (blob->type == DATA_BLOB_TYPE_TEXTURE
		|| blob->type == DATA_BLOB_TYPE_PALETTE
	)
	{
		LogDebug("texture/palette %08x -> %08x w/ %d refs"
			, blob->originalSegmentAddress
			, blob->updatedSegmentAddress
			, sb_count(blob->refs)
		);
		sb_foreach(blob->refs, {
			LogDebug(" -> %p, %08x %08x %08x"
				, (*each)
				, READ_32_BE(*each, -4)
				, READ_32_BE(*each, 0)
				, READ_32_BE(*each, 4)
			);
		});
	}
	*/
	
	sb_foreach(blob->refs, {
		if (*each)
			PUT_32_BE(*each, 0, blob->updatedSegmentAddress);
	});
}

void DataBlobApplyOriginalSegmentAddresses(struct DataBlob *blob)
{
	sb_foreach(blob->refs, {
		if (*each)
			PUT_32_BE(*each, 0, blob->originalSegmentAddress);
	});
}

void DataBlobPrint(struct DataBlob *blob)
{
	const char *typeName = 0;
	char extraInfo[256];
	
	extraInfo[0] = '\0';
	
	switch (blob->type)
	{
		case DATA_BLOB_TYPE_MESH:
			typeName = "mesh";
			sprintf(extraInfo, "sizeBytes = 0x%08x, has %d refs", blob->sizeBytes, sb_count(blob->refs));
			break;
		case DATA_BLOB_TYPE_VERTEX: typeName = "vertex"; break;
		case DATA_BLOB_TYPE_MATRIX: typeName = "matrix"; break;
		case DATA_BLOB_TYPE_TEXTURE:
			typeName = "texture";
			sprintf(extraInfo, "w/h/siz/fmt/linesize/pal/bytes = %d/%d/%d/%d/%d/%08x/%08x"
				, blob->data.texture.w
				, blob->data.texture.h
				, blob->data.texture.siz
				, blob->data.texture.fmt
				, blob->data.texture.lineSize
				, blob->data.texture.pal ? blob->data.texture.pal->originalSegmentAddress : 0
				, blob->sizeBytes
			);
			break;
		case DATA_BLOB_TYPE_PALETTE: typeName = "palette"; break;
		case DATA_BLOB_TYPE_GENERIC: typeName = "generic"; break;
		default: typeName = "unknown"; break;
	}
	
	if (blob->type != DATA_BLOB_TYPE_TEXTURE
		&& blob->type != DATA_BLOB_TYPE_PALETTE
	)
		typeName = 0;
	
	if (typeName)
		LogDebug("%s %08x%s%s"
			, typeName
			, blob->originalSegmentAddress
			, *extraInfo ? " : " : ""
			, *extraInfo ? extraInfo : ""
		);
}

void DataBlobPrintAll(struct DataBlob *blobs)
{
	for (struct DataBlob *blob = blobs; blob; blob = blob->next)
		DataBlobPrint(blob);
}
