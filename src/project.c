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
	
	// print vanilla folder
	if (proj->vanilla)
		LogDebug("vanilla = '%s'", proj->vanilla);
	
	FileFree(file);
	return proj;
}

void ProjectFree(struct Project *proj)
{
	free(proj->filename);
	free(proj->shortname);
	free(proj->folder);
	if (proj->vanilla) free(proj->vanilla);
	
	FileListFree(proj->foldersAll);
	FileListFree(proj->foldersObject);
	FileListFree(proj->foldersActor);
	FileListFree(proj->foldersActorSrc);
	
	free(proj);
}
