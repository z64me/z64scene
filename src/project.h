//
// project.h
//
// things pertaining to romhack projects
//

#ifndef PROJECT_H_INCLUDED
#define PROJECT_H_INCLUDED

#include "stretchy_buffer.h"

enum ProjectType
{
	PROJECT_TYPE_ZZROMTOOL,
	PROJECT_TYPE_ZZRTL,
	PROJECT_TYPE_Z64ROM,
};

struct Project
{
	char *filename;
	char *shortname;
	char *folder; // the containing folder
	char *vanilla;
	enum ProjectType type;
	
	sb_array(char *, foldersAll);
	sb_array(char *, foldersObject);
	sb_array(char *, foldersActor);
	sb_array(char *, foldersActorSrc);
};

struct Project *ProjectNewFromFilename(const char *filename);
void ProjectFree(struct Project *proj);

#endif // PROJECT_H_INCLUDED
