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
#endif

enum RenderGroup
{
	RENDERGROUP_IGNORE = 0,
	RENDERGROUP_ROOM = 0x01000000,
	RENDERGROUP_INST = 0x02000000,
	RENDERGROUP_MASK_ID = 0x00ffffff,
	RENDERGROUP_MASK_GROUP = 0xff000000,
};

enum GuiEnvPreviewMode
{
	GUI_ENV_PREVIEW_EACH,
	GUI_ENV_PREVIEW_TIME,
	GUI_ENV_PREVIEW_COUNT
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
	
	struct Instance *selectedInstance;
	sb_array(struct Instance, *instanceList); // used for duplicating etc
	
	sb_array(struct Instance, *actorList);
	sb_array(struct Instance, *doorList);
	sb_array(struct Instance, *spawnList);
	
	Vec3f newSpawnPos; // where a new inst/spawnpoint/etc will instantiate
	bool rightClickedInViewport;
	enum RenderGroup rightClickedRenderGroup;
	int selectedRoomIndex;
	
	bool clipboardHasInstance;
	struct Instance clipboardInstance;
};

#ifdef __cplusplus
}
#else

// C++ functions
void GuiInit(GLFWwindow *window);
void GuiCleanup(void);
void GuiDraw(GLFWwindow *window, struct Scene *scene, struct GuiInterop *interop);
int GuiHasFocus(void);
int GuiHasFocusMouse(void);
int GuiHasFocusKeyboard(void);
void GuiPushLine(int x1, int y1, int x2, int y2, uint32_t color, float thickness);
void GuiPushModal(const char *message);
struct ActorRenderCode *GuiGetActorRenderCode(uint16_t id);
void GuiCreateActorRenderCodeHandles(uint16_t id);
void GuiApplyActorRenderCodeProperties(struct Instance *inst);
int GuiGetActorObjectIdFromSlot(uint16_t actorId, int slot);
struct Object *GuiGetObjectDataFromId(int objectId);

#endif

#endif /* Z64SCENE_GUI_H_INCLUDED */

