//
// test.h
//
// where code is sometimes tested before being integrated
//

#include "misc.h"

void TestWren(void);
void Testz64convertScene(char **scenePath);
void Testz64convertObject(char **objectPath);
void TestAnalyzeSceneActors(struct Scene *scene, const char *logFilename);
void TestSaveLoadCycles(const char *filename);
void TestSwapFunction(void);
void TestSceneMigrate(const char *dstPath, const char *srcPath, const char *outPath);
