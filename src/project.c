//
// project.c
//
// things pertaining to romhack projects
//

#include "logging.h"
#include "project.h"
#include "file.h"
#include "misc.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static void ProjectParse_rom(struct Project *proj, struct File *file)
{
	const int cmdLen = 8; // 8 bytes per command
	const int searchAlign = 16;
	const uint8_t *dataStart = file->data;
	const uint8_t *dataEnd = file->dataEnd; dataEnd -= searchAlign;
	const uint8_t *searchEnd = dataEnd - searchAlign;
	
	for (const uint8_t *walk = dataStart; walk < searchEnd; walk += searchAlign)
	{
		// assume scenes always start with 0x15 or 0x18 followed by 0x15
		if (*walk != 0x15 && !(*walk == 0x18 && walk[cmdLen] == 0x15))
			continue;
		
		// if it's a scene header, it will reference a room list
		const uint8_t *sceneDataStart = walk;
		const uint8_t *roomList = 0;
		int numRooms = 0;
		for (const uint8_t *try = walk; try < searchEnd && *try != 0x14; try += cmdLen)
		{
			// scene header unlikely to be this long
			if (try >= sceneDataStart + 0x100)
				break;
			
			// scene header must contain room list
			if (*try == 0x04)
			{
				uint32_t w1 = u32r(try + 4) & 0x00ffffff;
				numRooms = try[1];
				
				if (try[4] != 0x02
					|| (w1 & 3) // points to unaligned data
					|| (w1 >= 1024 * 1024) // indicates file > 1mib, unlikely a scene
					|| numRooms == 0 // no rooms in list
					|| numRooms >= 64 // max rooms is 32, so unlikely to be valid
					|| dataEnd // room list runs past end of rom
						< sceneDataStart + w1 + numRooms * 8
				)
					break;
				
				roomList = sceneDataStart + w1;
			}
		} if (!roomList) continue; // no room list found
		
		// room list was found, so validate its contents
		for (int i = 0; i < numRooms; ++i)
		{
			const uint8_t *entry = roomList + i * 8;
			uint32_t start = u32r(entry);
			uint32_t end = u32r(entry + 4);
			
			if (end <= start
				|| start >= file->size
				|| end > file->size
				|| (start & 3) // unaligned room wouldn't work
				|| (end - start > 1024 * 1024) // 1 mib room size unlikely
				|| (dataStart[start] != 0x16 // assume rooms start with 0x16 or 0x18, 0x16
					&& !(dataStart[start] == 0x18 && dataStart[start + cmdLen] == 0x16)
				)
			) {
				roomList = 0;
				break;
			}
		} if (!roomList) continue; // invalidated based on contents
		
		//
		// try to find data resembling a reference to this scene
		//
		// a positive side effect of this is alternate headers stored
		// within a scene will be skipped, because only the first header
		// will pass this test
		//
		// additionally, while it's possible for there to be unreferenced
		// scenes in a modified rom (overwritten pointer w/ data still intact),
		// it's more likely that such scenes (or their rooms) are corrupt
		//
		// an earlier test for room headers in each room helps mitigate loading
		// potentially corrupt scenes, but still would not address scene size
		// the way this method does (although scene size isn't truly needed)
		//
		// the alternate header list(s) could be walked in an attempt to skip
		// them if an algorithm without the below check were desired
		//
		uint32_t sceneStart = sceneDataStart - dataStart;
		uint32_t sceneEnd = 0;
		for (const uint8_t *try = dataStart; try < dataEnd - 8; try += 4)
		{
			uint32_t w1 = u32r(try + 4);
			
			if (u32r(try) == sceneStart
				&& !(w1 & 3)
				&& w1 > sceneStart
				&& (w1 - sceneStart) < 1024 * 1024 // 1 mib scene size unlikely
			) {
				sceneEnd = w1;
				break;
			}
		} if (!sceneEnd) continue; // no matches found
		
		// add to list
		struct ProjectScene entry = {
			.startAddress = sceneStart,
			.endAddress = sceneEnd,
		};
		sb_push(proj->scenes, entry);
	}
}

static void ProjectParse_z64rom(struct Project *proj, const char *projToml)
{
	proj->type = PROJECT_TYPE_Z64ROM;
	
	sb_array(char *, folders) = proj->foldersAll;
	
	// determine name of vanilla subdirectory
	{
		const char *match = strstr(projToml, "vanilla =");
		if (match || (match = strstr(projToml, "vanilla=")))
		{
			match = MIN(strchr(match, '"'), strchr(match, '\n'));
			if (!match || *match != '"')
				goto L_defaultVanilla;
			match += 1;
			const char *matchEnd = MIN(strchr(match, '"'), strchr(match, '\n'));
			if (!matchEnd || *matchEnd != '"')
				goto L_defaultVanilla;
			int len = matchEnd - match;
			char *v = malloc(len + 1);
			memcpy(v, match, len);
			v[len] = '\0';
			proj->vanilla = v;
		}
		// sane default
		else
		{
		L_defaultVanilla:
			proj->vanilla = Strdup(".vanilla");
		}
	}
	
	proj->foldersObject = FileListFilterByWithVanilla(folders, "rom/object", proj->vanilla);
	proj->foldersActor = FileListFilterByWithVanilla(folders, "rom/actor", proj->vanilla);
	proj->foldersActorSrc = FileListFilterByWithVanilla(folders, "src/actor", proj->vanilla);
}

static void ProjectParse_zzrtl(struct Project *proj)
{
	char *zzrpl = 0;
	proj->type = PROJECT_TYPE_ZZRTL;
	
	sb_array(char *, folders) = proj->foldersAll;
	
	// load zzrpl
	{
		sb_array(char *, files) = FileListFromDirectory(proj->folder, 1, true, false, false);
		sb_array(char *, zzrpls) = FileListFilterBy(files, ".zzrpl", 0);
		
		if (sb_count(zzrpls) == 1)
		{
			struct File *zzrplFile = FileFromFilename(zzrpls[0]);
			zzrpl = Strdup(zzrplFile->data);
			FileFree(zzrplFile);
		}
		
		if (zzrpls)
			FileListFree(zzrpls);
		FileListFree(files);
	}
	
	// get game id from zzrpl
	if (strstr(zzrpl, "game"))
	{
		STRTOK_LOOP(zzrpl, "\r\n\t\" ")
		{
			if (!strcmp(each, "game")
				&& strlen(next) + 1 < sizeof(proj->game)
			)
				strcpy(proj->game, next);
		}
	}
	
	// determine name of vanilla subdirectory (if applicable)
	sb_foreach(folders, {
		const char *ref = *each;
		const char *match = strstr(ref, "/_vanilla");
		if (match) {
			match += 1;
			const char *matchEnd = strchr(match, '/');
			if (matchEnd) {
				int len = matchEnd - match;
				char *v = malloc(len + 1);
				memcpy(v, match, len);
				v[len] = '\0';
				proj->vanilla = v;
				break;
			}
		}
	})
	
	proj->foldersObject = FileListFilterByWithVanilla(folders, "object", proj->vanilla);
	proj->foldersActor = FileListFilterByWithVanilla(folders, "actor", proj->vanilla);
	proj->foldersActorSrc = FileListFilterByWithVanilla(folders, "actor", proj->vanilla);
	
	if (zzrpl)
		free(zzrpl);
}

struct Project *ProjectNewFromFilename(const char *filename)
{
	struct Project *proj = calloc(1, sizeof(*proj));
	struct File *file = FileFromFilename(filename);
	
	proj->filename = Strdup(file->filename);
	proj->shortname = Strdup(file->shortname);
	proj->folder = Strdup(file->filename);
	proj->folder[strlen(file->filename) - strlen(file->shortname)] = '\0';
	proj->foldersAll = FileListFromDirectory(proj->folder, 0, false, true, true);
	strcpy(proj->game, "oot"); // sane default
	
	// trim trailing '/' if present
	if (proj->folder[strlen(proj->folder) - 1] == '/')
		proj->folder[strlen(proj->folder) - 1] = '\0';
	
	// z64rom project
	if (file->size >= 10
		&& !memcmp(file->data, "[project]", 9)
	)
		ProjectParse_z64rom(proj, file->data);
	// zzrtl project
	else if (strstr(file->shortname, ".zzrpl")
		|| strstr(file->shortname, ".rtl")
	)
		ProjectParse_zzrtl(proj);
	else if (strstr(file->shortname, ".z64")
		&& strstr(file->shortname, ".z64")
			== strrchr(file->shortname, '.')
	)
		ProjectParse_rom(proj, file);
	
	// print vanilla folder
	if (proj->vanilla)
		LogDebug("vanilla = '%s'", proj->vanilla);
	
	proj->file = file;
	return proj;
}

void ProjectFree(struct Project *proj)
{
	free(proj->filename);
	free(proj->shortname);
	free(proj->folder);
	if (proj->vanilla) free(proj->vanilla);
	if (proj->file) FileFree(proj->file);
	
	FileListFree(proj->foldersAll);
	FileListFree(proj->foldersObject);
	FileListFree(proj->foldersActor);
	FileListFree(proj->foldersActorSrc);
	
	free(proj);
}
