//
// gui.h
//
// interop between gui and main program
//

#ifndef Z64SCENE_GUI_H_INCLUDED
#define Z64SCENE_GUI_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>
#include <GLFW/glfw3.h>

// C specific
#ifdef __cplusplus
extern "C" {
#include "texanim.h"
#include "object.h"
#define CPP_FUNC_PREFIX extern "C"
#else
#define CPP_FUNC_PREFIX
#endif

enum RenderGroup
{
	RENDERGROUP_IGNORE = 0,
	RENDERGROUP_ROOM = 0x01000000,
	RENDERGROUP_INST = 0x02000000,
	RENDERGROUP_PATHLINE = 0x04000000,
	RENDERGROUP_MASK_ID = 0x00ffffff,
	RENDERGROUP_MASK_GROUP = 0xff000000,
};

enum GuiEnvPreviewMode
{
	GUI_ENV_PREVIEW_EACH,
	GUI_ENV_PREVIEW_TIME,
	GUI_ENV_PREVIEW_COUNT
};

enum ZobjViewMode
{
	ZOBJ_VIEW_MODE_NONE = 0,
	ZOBJ_VIEW_MODE_MESH,
	ZOBJ_VIEW_MODE_SKELETON,
};

struct GuiInterop
{
	struct {
		bool isFogEnabled;
		bool isLightingEnabled;
		int envPreviewMode;
		int envPreviewEach;
		uint16_t envPreviewTime;
	} env;
	
	struct SceneHeader *sceneHeader;
	
	struct Instance *selectedInstance;
	sb_array(struct Instance, *instanceList); // used for duplicating etc
	
	sb_array(struct Instance, *actorList);
	sb_array(struct Instance, *doorList);
	sb_array(struct Instance, *spawnList);
	
	struct ObjectEntry *selectedObject;
	sb_array(struct ObjectEntry, *objectList);
	
	sb_array(struct AnimatedMaterial, *texanimList);
	
	sb_array(struct ObjectEntry, missingObjects);
	sb_array(struct ObjectEntry, unusedObjects);
	
	Vec3f newSpawnPos; // where a new inst/spawnpoint/etc will instantiate
	bool rightClickedInViewport;
	enum RenderGroup rightClickedRenderGroup;
	int rightClickedLineIndex;
	int rightClickedLinePathIndex;
	int selectedRoomIndex;
	int selectedHeaderIndex;
	int selectedDayIndex;
	uint16_t halfDayBits;
	bool isMM;
	bool hideUnselectedPaths;
	
	bool clipboardHasInstance;
	struct Instance clipboardInstance;
	
	int subkeepChildren; // how many children the current subkeep object has
	
	double deltaTimeSec;
	bool isZobjViewer;
	struct Object *zobj;
	int zobjCurrentDl;
	int zobjCurrentSkel;
	int zobjCurrentAnim;
	enum ZobjViewMode zobjViewMode;
};

#ifdef __cplusplus
}
#endif

// C++ functions
CPP_FUNC_PREFIX void GuiInit(GLFWwindow *window);
CPP_FUNC_PREFIX void GuiCleanup(void);
CPP_FUNC_PREFIX void GuiDraw(GLFWwindow *window, struct Scene *scene, struct GuiInterop *interop);
CPP_FUNC_PREFIX int GuiHasFocus(void);
CPP_FUNC_PREFIX int GuiHasFocusMouse(void);
CPP_FUNC_PREFIX int GuiHasFocusKeyboard(void);
CPP_FUNC_PREFIX void GuiPushLine(int x1, int y1, int x2, int y2, uint32_t color, float thickness);
CPP_FUNC_PREFIX void GuiPushPointX(int x, int y, uint32_t color, float thickness, int radius);
CPP_FUNC_PREFIX void GuiPushModal(const char *message);
CPP_FUNC_PREFIX struct ActorRenderCode *GuiGetActorRenderCode(uint16_t id);
CPP_FUNC_PREFIX void GuiCreateActorRenderCodeHandles(uint16_t id);
CPP_FUNC_PREFIX void GuiApplyActorRenderCodeProperties(struct Instance *inst);
CPP_FUNC_PREFIX int GuiGetActorObjectIdFromSlot(uint16_t actorId, int slot);
CPP_FUNC_PREFIX struct Object *GuiGetObjectDataFromId(int objectId);
CPP_FUNC_PREFIX void GuiLoadBaseDatabases(const char *gameId);
CPP_FUNC_PREFIX void GuiSetInterop(struct GuiInterop *interop);
CPP_FUNC_PREFIX void GuiErrorPopup(const char *message);
CPP_FUNC_PREFIX void GuiLoadProject(const char *fn);

#endif /* Z64SCENE_GUI_H_INCLUDED */

