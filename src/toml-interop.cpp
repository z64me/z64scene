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

static void DeriveNameFromFolderName(char **dst, const char *src)
{
	const char *tmp;
	// '/path/to/actor/0x0123 - My Actor' -> 'My Actor'
	if ((tmp = strrchr(src, '/'))
		&& (tmp = strchr(src, '-'))
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
	char tomlPath[1024];
	
	sb_foreach(project->foldersActorSrc, {
		const char *path = *each;
		uint16_t id = FileListFilePrefix(path);
		bool useTomlName = false;
		
		if (!id)
			continue;
		
		// load actor toml, if present
		snprintf(tomlPath, sizeof(tomlPath), "%s/actor.toml", path);
		if (FileExists(tomlPath)) {
			auto tmp = TomlLoadActorDatabaseEntry(tomlPath);
			tmp.index = id;
			actorDb->AddEntry(tmp);
			// if toml contains name, use that
			if (tmp.name)
				useTomlName = true;
			// could also derive object dependency from
			// zovl, but assume they're all in the toml
		}
		
		// update actor name to match folder name
		auto &entry = actorDb->GetEntry(id);
		if (useTomlName == false)
			DeriveNameFromFolderName(&entry.name, path);
	})
}

static void TomlInjectObjectsFromProject(Project *project, ObjectDatabase *objectDb)
{
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
	})
}

void TomlInjectDataFromProject(Project *project, ActorDatabase *actorDb, ObjectDatabase *objectDb)
{
	TomlInjectActorsFromProject(project, actorDb);
	TomlInjectObjectsFromProject(project, objectDb);
}
