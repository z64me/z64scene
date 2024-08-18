/*
 * fast64.c
 *
 * interop with output from the Fast64 Blender plugin
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "logging.h"
#include "fast64.h"
#include "file.h"
#include "misc.h"

#include <n64texconv.h>
#include <stb_image.h>

#define MIPS64_BINUTILS_PATH "" // TODO expose to user in gui

#ifdef _WIN32
	#define EXE_SUFFIX ".exe"
#else
	#define EXE_SUFFIX ""
#endif

#undef MIN
#undef MAX
#undef MIN3
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))

#define MIN3(A, B, C) MIN(A, MIN(B, C))

#define strcatf(X, ...) sprintf(X + strlen(X), __VA_ARGS__)

#define ARRAY_COUNT(X) (sizeof(X) / sizeof(*(X)))
#define N64TEXCONV_BPP_COUNT (N64TEXCONV_32 + 1)

#define WHERE_COMPILER_LOG  ExePath(WHERE_TMP "compiler.log")
#define WHERE_TMP_O         ExePath(WHERE_TMP "tmp.o")
#define WHERE_TMP_LD        ExePath(WHERE_TMP "tmp.ld")
#define WHERE_TMP_ELF       ExePath(WHERE_TMP "tmp.elf")

#define STRTOK_LOOP(STRING, DELIM) \
	for (char *next, *each = strtok(STRING, DELIM) \
		; each && (next = strtok(0, DELIM)) \
		; each = next \
	)

// z64ovl.ld
#if 1
static const char *z64ovlLd = R"(
	/* Libaries here -- add your own if you need to */
	SEARCH_DIR(/usr/mips/mips/lib)
	SEARCH_DIR(/usr/mips/lib/gcc/mips/3.4.6/)
	
	/* This script is for MIPS */
	OUTPUT_ARCH(mips)
	OUTPUT_FORMAT("elf32-bigmips", "elf32-bigmips", "elf32-littlemips")
	
	/* Entrypoint is function `n64start` */
	ENTRY(ENTRY_POINT)
	
	SECTIONS
	{
		/* Program run addressed defined in `entry.ld` */
		. = ENTRY_POINT;
		
		/* Machine code */
		.text ALIGN(4):
		{
			/* Pad with NULL */
			FILL(0);
			
			/* Address to start of text section */
			__text_start = . ;
			
			/* Data goes here */
			*(.init);
			*(.text);
			*(.ctors);
			*(.dtors);
			*(.fini);
			
			/* Address to end of text section */
			. = ALIGN(16);
			__text_end = . ;
		}
		
		/* Initialized data */
		.data ALIGN(16):
		{
			/* Pad with NULL */
			FILL(0);
			
			/* Address to start of data section */
			__data_start = . ;
			
			/* Data goes here*/
			*(.data);
			*(.data.*);
			
			/* Data pointer */
			. = ALIGN(8);
			_gp = . ;
			*(.sdata);
			
			/* Address to end of data section */
			. = ALIGN(16);
			__data_end = . ;
		}
		
		/* Read-only data */
		.rodata ALIGN(16):
		{
			/* Pad with NULL */
			FILL(0);
			
			/* Address to start of rodata section */
			__rodata_start = . ;
			
			/* Data goes here*/
			*(.rodata);
			*(.rodata.*);
			*(.eh_frame);
			
			/* Address to end of rodata section */
			. = ALIGN(16);
			__rodata_end = . ;
		}
		
		/* Memory initialized to zero */
		.bss ALIGN(8):
		{
			/* Address to start of BSS section */
			__bss_start = . ;
			
			/* BSS data */
			*(.scommon);
			*(.sbss);
			*(.bss);
			
			/* Address to end of BSS section */
			. = ALIGN(8);
			__bss_end = . ;
		}
		
		/* End of our memory use */
		. = ALIGN(8);
		end = . ;
	}
)";
#endif

// C sources for scene and each room within (no specific order)
static sb_array(struct File, sCfiles) = 0;

static const char *Error(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));

static const char *Error(const char *fmt, ...)
{
	static char *work = 0;
	const size_t workSize = 4096;
	va_list args;
	
	if (!work)
		work = malloc(workSize);
	
	va_start(args, fmt);
		vsnprintf(work, workSize, fmt, args);
	va_end(args);
	
	return work;
}

static const char *ErrorCompilerLog(const char *source)
{
	const char *error;
	struct File *tmp = FileFromFilename(WHERE_COMPILER_LOG);
	if (tmp->size >= 1024 * 3) // TODO hardcoding, must be smaller than Error.workSize
		((char*)tmp->data)[1024 * 3] = '\0';
	error = Error("%s compilation error: %s", source, (char*)(tmp->data));
	FileFree(tmp);
	return error;
}

static bool IsSceneRoomSource(const char *path)
{
	if (strstr(path, ".inc") // skip .inc and .inc.c files
		|| strlen(strrchr(path, '.')) != 2 // must end with '.c'
		|| (!strstr(path, "_scene") // test_scene.c
			&& !strstr(path, "_room_") // test_room_%d.c
		)
	)
		return false;
	
	return true;
}

static const char *Fast64_CompileTexture(const char *imagePath)
{
	int width;
	int height;
	int channels;
	unsigned int sz;
	void *image = 0;
	void *pal = 0;
	int palColors = 0;
	static char workBuf[2048];
	const char *formatString;
	const char *formatStringStart;
	const char *texconvErr;
	enum n64texconv_fmt fmt = N64TEXCONV_FMT_MAX;
	enum n64texconv_bpp bpp = N64TEXCONV_BPP_COUNT;
	const char *names[] = {
		[N64TEXCONV_RGBA] = "rgba",
		[N64TEXCONV_YUV] = "yuv",
		[N64TEXCONV_CI] = "ci",
		[N64TEXCONV_IA] = "ia",
		[N64TEXCONV_I] = "i",
	};
	const char *bpps[] = {
		[N64TEXCONV_4] = "4",
		[N64TEXCONV_8] = "8",
		[N64TEXCONV_16] = "16",
		[N64TEXCONV_32] = "32",
	};
	#define CLEANUP \
		if (image) stbi_image_free(image); \
		if (pal) stbi_image_free(pal); \
	
	image = stbi_load(imagePath, &width, &height, &channels, STBI_rgb_alpha);
	if (!image)
	{
		CLEANUP
		return Error("failed to load image file '%s'", imagePath);
	}
	
	formatString = strrchr(imagePath, '.');
	if (formatString) while (*(--formatString) != '.');
	if (!formatString)
	{
		CLEANUP
		return Error("failed to find format string in filename '%s'", imagePath);
	}
	
	formatString += 1; // skip the '.'
	formatStringStart = formatString;
	for (int i = 0; i < ARRAY_COUNT(names); ++i)
		if (names[i] && !strncasecmp(formatString, names[i], strlen(names[i]))) {
			formatString += strlen(names[i]);
			fmt = i;
			break;
		}
	for (int i = 0; i < ARRAY_COUNT(bpps); ++i)
		if (bpps[i] && !strncasecmp(formatString, bpps[i], strlen(bpps[i]))) {
			formatString += strlen(bpps[i]);
			bpp = i;
			break;
		}
	
	if (*formatString != '.'
		|| fmt == N64TEXCONV_FMT_MAX
		|| bpp == N64TEXCONV_BPP_COUNT
	)
	{
		CLEANUP
		return Error(
			"failed to parse format string '%.*s' in filename '%s'",
			(int)strcspn(formatStringStart, "."), formatStringStart, imagePath
		);
	}
	
	// getting tlut (color palette) associated with
	// indexed textures involves parsing source code
	if (fmt == N64TEXCONV_CI)
	{
		char *varName = workBuf;
		const char *tmp = MAX(strrchr(imagePath, '/'), strrchr(imagePath, '\\'));
		const char *match = 0;
		
		strcpy(varName, tmp + 1);
		strcpy(strchr(varName, '.'), ","); // 'test_sceneTex_name,' (comma for finding ref's instead of decl's)
		sb_foreach(sCfiles, {
			if ((match = strstr(each->data, varName)))
				break;
		})
		
		if (match)
		{
			const char *startBrace = match; while (*startBrace != '{') --startBrace;
			const char *endBrace = strchr(match, '}');
			const char *endTri = strstr(match, "gsSP1Triangle");
			const char *endTri2 = strstr(match, "gsSP2Triangles");
			if (!endTri) endTri = endBrace;
			if (!endTri2) endTri2 = endBrace;
			const char *end = MIN3(endBrace, endTri, endTri2);
			const char *loadTlut = 0;
			
			for (const char *work = startBrace; ; )
			{
				work = strstr(work + 1, "gsDPLoadTLUT_pal");
				
				if (work && work < end)
					loadTlut = work;
				else
					break;
			}
			
			if (loadTlut)
			{
				char *palettePath = workBuf;
				char *tmp;
				loadTlut = strchr(loadTlut, '(');
				while (*loadTlut && !isalnum(*loadTlut))
					++loadTlut;
				if (isdigit(*loadTlut)) {
					loadTlut = strchr(loadTlut, ',');
					while (*loadTlut && !isalnum(*loadTlut))
						++loadTlut;
				}
				strcpy(palettePath, imagePath);
				tmp = MAX(strrchr(palettePath, '/'), strrchr(palettePath, '\\')) + 1;
				while (*loadTlut && (isalnum(*loadTlut) || *loadTlut == '_')) {
					*tmp = *loadTlut;
					++tmp;
					++loadTlut;
				}
				sprintf(tmp, ".rgba16.png");
				int w, h, c;
				int palFmt = N64TEXCONV_RGBA;
				int palBpp = N64TEXCONV_16;
				// support ia16 tlut (uncommon)
				// TODO: untested, so please test; may require changes in n64texconv as well
				if (!(pal = stbi_load(palettePath, &w, &h, &c, STBI_rgb_alpha)))
				{
					sprintf(tmp, ".ia16.png");
					palFmt = N64TEXCONV_IA;
				}
				palColors = w * h;
				texconvErr = n64texconv_to_n64(pal, pal, 0, 0, palFmt, palBpp, w, h, &sz);
				if (texconvErr)
				{
					CLEANUP
					return Error(
						"failed to convert texture '%s'; reason: '%s'",
						palettePath, texconvErr
					);
				}
			}
			else
			{
				goto L_errorNoTlut;
			}
		}
		else
		{
		L_errorNoTlut:
			CLEANUP
			return Error("failed to find tlut for texture '%s'", imagePath);
		}
	}
	
	texconvErr = n64texconv_to_n64(image, image, pal, palColors, fmt, bpp, width, height, &sz);
	if (texconvErr)
	{
		CLEANUP
		return Error(
			"failed to convert texture '%s'; reason: '%s'",
			imagePath, texconvErr
		);
	}
	
	// path/to/texture.format.png -> path/to/texture.format.inc.c
	strcpy(workBuf, imagePath);
	sprintf(strrchr(workBuf, '.'), ".inc.c");
	FILE *fp = fopen(workBuf, "wb");
	for (unsigned int i = 0; i < sz; i += 8)
	{
		const uint8_t *x = image;
		x += i;
		
		fprintf(fp, "0x%02x%02x%02x%02x%02x%02x%02x%02x,\n",
			x[0], x[1], x[2], x[3], x[4], x[5], x[6], x[7]
		);
	}
	fclose(fp);
	
	LogDebug("texture '%s' converted", imagePath);
	
	CLEANUP
	return 0;
}

static const char *Fast64_CompileTextures(sb_array(char *, imagePaths))
{
	sb_foreach(imagePaths, {
		const char *err = Fast64_CompileTexture(*each);
		if (err)
			return err;
	})
	return 0;
}

static const char *Fast64_CompileSource(const char *syms, const char *root, const char *source, const char *out)
{
	const char *error = 0;
	char command[4096];
	
	// generate linker script .ld
	FILE *fp = fopen(WHERE_TMP_LD, "wb");
	fprintf(fp, "%s\n", syms);
	fprintf(fp, "%s\n", z64ovlLd);
	fclose(fp);
	
	// compile .c -> .o
	sprintf(command,
		"%s""mips64-gcc" EXE_SUFFIX
		" -G0"
		" -o \"%s\""
		" -c \"%s\""
		" -fno-zero-initialized-in-bss"
		" -fno-toplevel-reorder"
		" -I\"%s/include\""
		" -I\"%s/.\""
		" 2> \"%s\"",
		MIPS64_BINUTILS_PATH, WHERE_TMP_O, source, root, root, WHERE_COMPILER_LOG
	);
	if (system(command))
		return ErrorCompilerLog(source);
	
	// link .o -> .elf
	sprintf(command,
		"%s""mips64-ld" EXE_SUFFIX " --emit-relocs -o \"%s\" \"%s\" -T \"%s\" 2> \"%s\"",
		MIPS64_BINUTILS_PATH, WHERE_TMP_ELF, WHERE_TMP_O, WHERE_TMP_LD, WHERE_COMPILER_LOG
	);
	if (system(command))
		return ErrorCompilerLog(source);
	
	// dump .elf -> .bin
	sprintf(command,
		"%s""mips64-objcopy" EXE_SUFFIX " -R .MIPS.abiflags -O binary \"%s\" \"%s\" 2> \"%s\"",
		MIPS64_BINUTILS_PATH, WHERE_TMP_ELF, ExePath(out), WHERE_COMPILER_LOG
	);
	if (system(command))
		return ErrorCompilerLog(source);
	
	return error;
}

const char *Fast64_CompileSceneAndRooms(const char *root, sb_array(char *, sourcePaths))
{
	const char *error = 0;
	static char *syms = 0;
	
	if (!syms)
		syms = malloc(16 * 1024); // 16 kib
	
	// find and compile the scene
	const char *scenePath = 0;
	sb_foreach(sourcePaths, {
		const char *path = *each;
		if (!IsSceneRoomSource(path))
			continue;
		if (strstr(path, "_scene.c"))
			scenePath = path;
	})
	if (!scenePath && (error = "failed to find scene"))
		goto L_earlyExit;
	
	// get symbols
	struct File *file = FileFromFilename(scenePath);
	char sym[256];
	strcpy(syms, "ENTRY_POINT = 0x02000000;\n");
	for (const char *tmp = file->data; tmp; )
	{
		tmp = strstr(tmp, "SegmentRom");
		
		if (!tmp)
			break;
		while (isalnum(*tmp) || (*tmp == '_'))
			--tmp;
		
		// symbol
		int i = 0;
		for (i = 0, ++tmp; isalnum(*tmp) || *tmp == '_'; ++i, ++tmp)
			sym[i] = *tmp;
		sym[i] = '\0';
		strcatf(syms, "%s = 0;\n", sym);
	}
	FileFree(file);
	
	if ((error = Fast64_CompileSource(syms, root, scenePath, WHERE_TMP "test_scene.zscene")))
		goto L_earlyExit;
	
	// dump symbols
	sprintf(syms,
		"%s""mips64-objdump" EXE_SUFFIX " -t \"%s\" > \"%s\"",
		MIPS64_BINUTILS_PATH, WHERE_TMP_ELF, WHERE_COMPILER_LOG
	);
	if (system(syms))
		return ErrorCompilerLog(scenePath);
	
	// get symbols
	strcpy(syms, "ENTRY_POINT = 0x03000000;\n");
	file = FileFromFilename(WHERE_COMPILER_LOG);
	STRTOK_LOOP((char*)file->data, "\r\n") {
		unsigned int addr;
		const char *name;
		
		if (!strstr(each, " O ")
			|| !isdigit(*each)
			|| sscanf(each, "%X", &addr) != 1
			|| !(name = strrchr(each, ' '))
		)
			continue;
		
		strcatf(syms, "%s = 0x%08X;\n", name + 1, addr);
	}
	FileFree(file);
	
	// find and compile the rooms
	char *fn = syms + strlen(syms) + 1;
	sb_foreach(sourcePaths, {
		const char *path = *each;
		const char *room;
		int idx;
		if (!IsSceneRoomSource(path))
			continue;
		if ((room = strstr(path, "_room_"))) {
			sscanf(room, "_room_%d", &idx);
			sprintf(fn, "%s" "test_room_%d.zmap", WHERE_TMP, idx);
			if ((error = Fast64_CompileSource(syms, root, path, fn)))
				goto L_earlyExit;
		}
	})
	
L_earlyExit:
	return error;
}

const char *Fast64_Compile(const char *path)
{
	sb_array(char *, filenames);
	sb_array(char *, imagePaths);
	sb_array(char *, sourcePaths);
	char *folder = strdup(path);
	char *root = strdup(path);
	char *tmp = MAX(strrchr(folder, '/'), strrchr(folder, '\\'));
	const char *error = 0;
	
	if (!tmp)
		return Error("failed to determine directory of '%s'", path);
	
	// determine root
	{
		char *x = strstr(root, "/assets/");
		if (!x)
			return Error(
				"could not find '/assets/' in '%s' (not a decomp directory?)"
				, path
			);
		x[0] = '\0';
	}
	
	*tmp = '\0';
	filenames = FileListFromDirectory(folder, 0, true, false, false);
	imagePaths = FileListFilterBy(filenames, ".png", 0);
	sourcePaths = FileListFilterBy(filenames, ".c", 0);
	
	sb_foreach(sourcePaths, {
		const char *path = *each;
		if (!IsSceneRoomSource(path))
			continue;
		
		struct File *file = FileFromFilename(path);
		file->ownsHandle = false;
		sb_push(sCfiles, *file);
		LogDebug("add compilation unit '%s'", path);
		free(file);
	})
	
	FileListPrintAll(imagePaths);
	
	error = Fast64_CompileTextures(imagePaths);
	if (error)
		goto L_earlyExit;
	
	if ((error = Fast64_CompileSceneAndRooms(root, sourcePaths)))
		goto L_earlyExit;
	
L_earlyExit:
	sb_foreach(sCfiles, { FileFree(each); })
	sb_clear(sCfiles);
	FileListFree(sourcePaths);
	FileListFree(imagePaths);
	FileListFree(filenames);
	free(folder);
	free(root);
	
	return error;
}
