//
// toml-interop.cpp <z64scene>
//
// separate stuff for copying info
// from romhack project subdirs
// into toml actor/object databases
//

#include "toml-parsers.hpp"
extern "C" {
#include "project.h"
#include "file.h"
};

#define STRTOK_LOOP(STRING, DELIM) \
	for (char *next, *each = strtok(STRING, DELIM) \
		; each && (next = strtok(0, DELIM)) \
		; each = next \
	)

const char *FindMatchingFile(const char *path, const char *defaultFilename)
{
	static char workbuf[1024];
	
	// cheaper
	snprintf(workbuf, sizeof(workbuf), "%s/%s", path, defaultFilename);
	if (FileExists(workbuf))
		return workbuf;
	
	// more exhaustive
	const char *extension = strrchr(defaultFilename, '.');
	int extensionLen = strlen(extension);
	sb_array(char *, files) = FileListFromDirectory(path, 1, true, false, false);
	sb_array(char *, filtered) = FileListFilterBy(files, extension, 0);
	workbuf[0] = '\0';
	// TODO FileListFilterBy() filtering by extension so no loop necessary
	//if (sb_count(filtered))
	//	strcpy(workbuf, filtered[0]);
	sb_foreach(filtered, {
		const char *tmp = *each;
		// guarantees extension at end of filename, so no .extension.bak
		if ((tmp = strstr(tmp, extension))
			&& strlen(tmp) == extensionLen
		) {
			strcpy(workbuf, *each);
			break;
		}
	})
	FileListFree(filtered);
	FileListFree(files);
	return workbuf[0] ? workbuf : 0;
}

static void DeriveNameFromFolderName(char **dst, const char *src)
{
	const char *tmp;
	// '/path/to/actor/0x0123 - My Actor' -> 'My Actor'
	if ((tmp = strrchr(src, '/'))
		&& (tmp = strchr(tmp, '-'))
	) {
		if (*dst) {
			free(*dst);
			*dst = 0;
		}
		while (tmp && *tmp && !isalnum(*tmp))
			++tmp;
		if (*tmp)
			*dst = strdup(tmp);
	}
	// failed to derive name, and no name to fall back to
	if (!*dst || !**dst)
		*dst = strdup("custom? unset");
}

static void TomlInjectActorsFromProject(Project *project, ActorDatabase *actorDb)
{
	// TODO support deleting actors that don't exist in project
	
	char tomlPath[1024];
	
	sb_foreach(project->foldersActorSrc, {
		const char *path = *each;
		const char *pathBinary = path;
		uint16_t id = FileListFilePrefix(path);
		bool useTomlName = false;
		
		if (!id)
			continue;
		
		// z64rom stores compiled overlays in another folder
		if (project->type == PROJECT_TYPE_Z64ROM
			&& !(pathBinary = FileListFindPathToId(project->foldersActor, id))
		) continue;
		
		// load actor toml, if present
		snprintf(tomlPath, sizeof(tomlPath), "%s/actor.toml", path);
		if (FileExists(tomlPath)) {
			auto tmp = TomlLoadActorDatabaseEntry(tomlPath);
			tmp.index = id;
			actorDb->AddEntry(tmp);
			// if toml contains name, use that
			if (tmp.name)
				useTomlName = true;
		}
		
		// update actor name to match folder name
		auto &entry = actorDb->GetEntry(id);
		if (useTomlName == false)
		{
			DeriveNameFromFolderName(&entry.name, path);
			
			fprintf(stderr, "derived actor id %04x name = '%s'\n", id, entry.name);
		}
		
		/*
		if (useTomlName == true)
		{
			fprintf(stderr, "tomlName = '%s'\n", entry.name);
			fprintf(stderr, "objects.size = %d\n", entry.objects.size());
			fprintf(stderr, "objects.[0] = %d\n", entry.objects[0]);
			exit(0);
		}
		*/
		
		// if no object dependency, derive it from zovl
		if (entry.objects.size() == 0
			|| (entry.objects.size() == 1
				&& entry.objects[0] == 0x0001
			)
		)
		{
			const char *delim = "\r\n\t =";
			uint32_t vram = 0;
			uint32_t ivar = 0;
			
			if (project->type == PROJECT_TYPE_ZZRTL) {
				snprintf(tomlPath, sizeof(tomlPath), "%s/conf.txt", path);
				if (FileExists(tomlPath)) {
					File *file = FileFromFilename(tomlPath);
					STRTOK_LOOP((char*)file->data, delim) {
						if (!strcmp(each, "vram")) sscanf(next, "%x", &vram);
						if (!strcmp(each, "ivar")) sscanf(next, "%x", &ivar);
					}
					FileFree(file);
				}
				// ivar override
				snprintf(tomlPath, sizeof(tomlPath), "%s/ZzrtlInitVars.txt", path);
				if (FileExists(tomlPath)) {
					File *file = FileFromFilename(tomlPath);
					sscanf((char*)file->data, "%x", &ivar);
					FileFree(file);
				}
			}
			else if (project->type == PROJECT_TYPE_Z64ROM) {
				snprintf(tomlPath, sizeof(tomlPath), "%s/config.toml", pathBinary);
				if (FileExists(tomlPath)) {
					File *file = FileFromFilename(tomlPath);
					STRTOK_LOOP((char*)file->data, delim) {
						if (!strcmp(each, "vram_addr")) sscanf(next, "%x", &vram);
						if (!strcmp(each, "init_vars")) sscanf(next, "%x", &ivar);
					}
					FileFree(file);
				}
			}
			
			// safety
			if (vram >= 0x80800000 && ivar >= 0x80800000
				&& vram < 0x81000000 && ivar < 0x81000000
				&& ivar >= vram
			) {
				const char *zovlFilename = FindMatchingFile(
					pathBinary
					, project->type == PROJECT_TYPE_Z64ROM
						? "overlay.zovl"
						: "actor.zovl"
				);
				
				// found an overlay
				if (zovlFilename) {
					fprintf(stderr, "zovlFilename = '%s'\n", zovlFilename);
					File *file = FileFromFilename(zovlFilename);
					ivar -= vram;
					// initialization data is within the file
					if (ivar + 0x20 < file->size) {
						const uint8_t *data = (const uint8_t*)file->data;
						data += ivar;
						uint16_t objId = (data[8] << 8) | data[9];
						fprintf(stderr, "derived object dependency %04x\n", objId);
						if (entry.objects.size() == 0)
							entry.objects.emplace_back(objId);
						else
							entry.objects[0] = objId;
					}
					FileFree(file);
				}
			}
		}
	})
}

static void TomlInjectObjectsFromProject(Project *project, ObjectDatabase *objectDb)
{
	// TODO support deleting objects that don't exist in project
	
	sb_foreach(project->foldersObject, {
		const char *path = *each;
		uint16_t id = FileListFilePrefix(path);
		
		if (!id)
			continue;
		
		// if index not already populated, add one
		if (objectDb->GetEntry(id).isEmpty) {
			ObjectDatabase::Entry entry = {0};
			entry.index = id;
			objectDb->AddEntry(entry);
		}
		
		// update object name to match folder name
		auto &entry = objectDb->GetEntry(id);
		DeriveNameFromFolderName(&entry.name, path);
		
		const char *zobjFilename = FindMatchingFile(
			path
			, project->type == PROJECT_TYPE_Z64ROM
				? "object.zobj"
				: "zobj.zobj"
		);
		if (zobjFilename)
			entry.zobjPath = strdup(zobjFilename);
		
		fprintf(stderr, "derived object id %04x name = '%s'\n", id, entry.name);
		if (entry.zobjPath)
			fprintf(stderr, " with filename = '%s'\n", entry.zobjPath);
	})
}

void TomlInjectDataFromProject(Project *project, ActorDatabase *actorDb, ObjectDatabase *objectDb)
{
	TomlInjectActorsFromProject(project, actorDb);
	TomlInjectObjectsFromProject(project, objectDb);
}
