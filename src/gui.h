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
	
	struct Instance *queueInstanceSelect;
	struct Instance *selectedInstance;
	sb_array(struct Instance, *instanceList)[INSTANCE_TAB_COUNT];
	
	Vec3f newSpawnPos; // where a new inst/spawnpoint/etc will instantiate
	bool rightClickedInViewport;
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

#endif

#endif /* Z64SCENE_GUI_H_INCLUDED */

