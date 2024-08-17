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

#include <n64texconv.h>
#include <stb_image.h>

#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))

#define MIN3(A, B, C) MIN(A, MIN(B, C))

#define ARRAY_COUNT(X) (sizeof(X) / sizeof(*(X)))
#define N64TEXCONV_BPP_COUNT (N64TEXCONV_32 + 1)

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

const char *Fast64_Compile(const char *path)
{
	sb_array(char *, filenames);
	sb_array(char *, imagePaths);
	sb_array(char *, sourcePaths);
	char *folder = strdup(path);
	char *tmp = MAX(strrchr(folder, '/'), strrchr(folder, '\\'));
	const char *error = 0;
	
	if (!tmp)
		return Error("failed to determine directory of '%s'", path);
	
	*tmp = '\0';
	filenames = FileListFromDirectory(folder, 0, true, false, false);
	imagePaths = FileListFilterBy(filenames, ".png", 0);
	sourcePaths = FileListFilterBy(filenames, ".c", 0);
	
	sb_foreach(sourcePaths, {
		const char *path = *each;
		if (strstr(path, ".inc") // skip .inc and .inc.c files
			|| strlen(strrchr(path, '.')) != 2 // must end with '.c'
			|| (!strstr(path, "_scene") // test_scene.c
				&& !strstr(path, "_room_") // test_room_%d.c
			)
		)
			continue;
		
		struct File *file = FileFromFilename(path);
		file->ownsHandle = false;
		sb_push(sCfiles, *file);
		LogDebug("add compilation unit '%s'", path);
		free(file);
	})
	
	FileListPrintAll(imagePaths);
	
	error = Fast64_CompileTextures(imagePaths);
	
	sb_foreach(sCfiles, { FileFree(each); })
	sb_clear(sCfiles);
	FileListFree(sourcePaths);
	FileListFree(imagePaths);
	FileListFree(filenames);
	free(folder);
	
	return error;
}
