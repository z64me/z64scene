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
#include "stretchy_buffer.h"

#define F3DEX_GBI_2
#include "gbi.h"

#define Min(A, B) ((A) < (B) ? (A) : (B))

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
#define ShiftR SHIFTR

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
	struct DataBlob *prev = listHead;
	
	for (struct DataBlob *each = listHead; each; each = each->next)
	{
		// already in list
		if (each->originalSegmentAddress == segmentAddr)
		{
			// it's possible for blocks to be coalesced
			if (sizeBytes > each->sizeBytes)
				each->sizeBytes = sizeBytes;
			
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

void DataBlobSegmentSetup(int segmentIndex, const void *data, const void *dataEnd, struct DataBlob *head)
{
	struct DataBlobSegment *seg = &gSegments[segmentIndex];
	
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

struct DataBlob *DataBlobSegmentPush(
	const void *refData
	, uint32_t sizeBytes
	, uint32_t segmentAddr
	, enum DataBlobType type
)
{
	struct DataBlobSegment *seg = DataBlobSegmentGet(segmentAddr >> 24);
	
	if (!seg)
		return 0;
	
	seg->head = DataBlobPush(seg->head, refData, sizeBytes, segmentAddr, type);
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
	
	static struct DataBlob *palBlob = 0;
	static sb_array(struct DataBlob *, needsPalettes); // color-indexed textures w/o palettes
	
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
						struct DataBlob *blob;
						
						// old method was returning 0 here
						if (siz == G_IM_SIZ_4b)
							size = (width * height) / 2;
						else
							size = G_SIZ_BYTES(siz) * width * height;
						
						if (size > 4096)
							fprintf(stderr, "warning: width height %d x %d\n", width, height);
						
						blob = DataBlobSegmentPush(realAddr, size, addr, DATA_BLOB_TYPE_TEXTURE);
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
						
						palBlob = DataBlobSegmentPush(realAddr, size, addr, DATA_BLOB_TYPE_PALETTE);
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

// new method, based on cooliscool's uot: https://code.google.com/p/uot
// TODO nested functions are not standard c99, so refactor this eventually
void DataBlobSegmentsPopulateFromMeshNew(uint32_t segAddr)
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
	
	static struct {
		uint32_t Dram;
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
				DataBlobSegmentsPopulateFromMeshNew(w1);
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
				}
				break;
			
			case G_MTX:
				if ((realAddr = DataBlobSegmentAddressToRealAddress(w1)))
					DataBlobSegmentPush(realAddr, SIZEOF_MTX, w1, DATA_BLOB_TYPE_MATRIX);
				break;
			
			case G_VTX:
				if ((realAddr = DataBlobSegmentAddressToRealAddress(w1)))
					DataBlobSegmentPush(realAddr, (SHIFTR(w0, 12, 8)) * SIZEOF_VTX, w1, DATA_BLOB_TYPE_VERTEX);
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
					Textures(0).Dram = w1;
				else
					Textures(CurrentTex).Dram = w1;
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
				}
				break;
			
			case G_LOADTILE:
			case G_LOADBLOCK:
				// These commands are only used inside larger texture macros, so we don't need to do anything special with them
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
						fprintf(stderr, "warning: width height %d x %d\n", width, height);
					
					blob = DataBlobSegmentPush(realAddr, size, addr, DATA_BLOB_TYPE_TEXTURE);
					blob->data.texture.w = width;
					blob->data.texture.h = height;
					blob->data.texture.siz = siz;
					blob->data.texture.fmt = Textures(CurrentTex).TexFormat;
					if (blob->data.texture.fmt == G_IM_FMT_CI
						&& blob->data.texture.pal == 0
					)
						sb_push(needsPalettes, blob);
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
						
						palBlob = DataBlobSegmentPush(realAddr, size, addr, DATA_BLOB_TYPE_PALETTE);
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
}

void DataBlobPrint(struct DataBlob *blob)
{
	const char *typeName = 0;
	char extraInfo[256];
	
	extraInfo[0] = '\0';
	
	switch (blob->type)
	{
		case DATA_BLOB_TYPE_MESH: typeName = "mesh"; break;
		case DATA_BLOB_TYPE_VERTEX: typeName = "vertex"; break;
		case DATA_BLOB_TYPE_MATRIX: typeName = "matrix"; break;
		case DATA_BLOB_TYPE_TEXTURE:
			typeName = "texture";
			sprintf(extraInfo, "w/h/siz/fmt/pal = %d/%d/%d/%d/%08x"
				, blob->data.texture.w
				, blob->data.texture.h
				, blob->data.texture.siz
				, blob->data.texture.fmt
				, blob->data.texture.pal ? blob->data.texture.pal->originalSegmentAddress : 0
			);
			break;
		case DATA_BLOB_TYPE_PALETTE: typeName = "palette"; break;
		case DATA_BLOB_TYPE_GENERIC: typeName = "generic"; break;
		default: typeName = "unknown"; break;
	}
	
	if (typeName)
		fprintf(stderr, "%s %08x%s%s\n"
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
