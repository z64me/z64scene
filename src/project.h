//
// project.h
//
// things pertaining to romhack projects
//

#ifndef PROJECT_H_INCLUDED
#define PROJECT_H_INCLUDED

#include "stretchy_buffer.h"

#define STRTOK_LOOP(STRING, DELIM) \
	for (char *next, *each = strtok(STRING, DELIM) \
		; each && (next = strtok(0, DELIM)) \
		; each = next \
	)

enum ProjectType
{
	PROJECT_TYPE_ZZROMTOOL,
	PROJECT_TYPE_ZZRTL,
	PROJECT_TYPE_Z64ROM,
	PROJECT_TYPE_ROM,
};

struct ProjectScene
{
	char *name;
	int id;
	
	uint32_t startAddress;
	uint32_t endAddress;
	uint32_t sizeBytes;
};

struct Project
{
	char *filename;
	char *shortname;
	char *folder; // the containing folder
	char *vanilla;
	enum ProjectType type;
	char game[16];
	
	sb_array(char *, foldersAll);
	sb_array(char *, foldersObject);
	sb_array(char *, foldersActor);
	sb_array(char *, foldersActorSrc);
	
	sb_array(struct ProjectScene, scenes);
	struct File *file;
};

struct Project *ProjectNewFromFilename(const char *filename);
void ProjectFree(struct Project *proj);

#endif // PROJECT_H_INCLUDED
