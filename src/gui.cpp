#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>
#include <functional>
#include <climits>
#include "misc.h"
#include "gui.h"

#if 1 // region: helper macros

#define CLAMP(VALUE, MIN, MAX) (((VALUE) < (MIN)) ? (MIN) : ((VALUE) > (MAX)) ? (MAX) : (VALUE))
#define WRAP(VALUE, MIN, MAX) (((VALUE) < (MIN)) ? (MAX) : ((VALUE) > (MAX)) ? (MIN) : (VALUE))

// allows using mousewheel while hovering the last-drawn combo box to quickly change its value
#define IMGUI_COMBO_HOVER(CURRENT, HOWMANY) \
	if (ImGui::IsItemHovered() && ImGui::GetIO().MouseWheel) \
		CURRENT = WRAP((CURRENT) - ImGui::GetIO().MouseWheel, 0, (HOWMANY) - 1);

#endif

#if 1 // region: private types

struct GuiSettings
{
	bool showSidebar = true;
	bool showImGuiDemoWindow = false;
	
	// combo boxes can be volatile as sizes change
	// when loading different scenes for editing
	struct
	{
		int instanceCurrent;
	} combos;
	
	void Reset(void)
	{
		memset(&combos, 0, sizeof(combos));
	}
	
private:
	struct Line
	{
		int x1;
		int y1;
		int x2;
		int y2;
		uint32_t color;
		float thickness;
		
		Line(int x1, int y1, int x2, int y2, uint32_t color, float thickness)
		{
			this->x1 = x1;
			this->y1 = y1;
			this->x2 = x2;
			this->y2 = y2;
			this->color = color;
			this->thickness = thickness;
		}
	};
	std::vector<Line> lines;
public:
	void PushLine(int x1, int y1, int x2, int y2, uint32_t color, float thickness)
	{
		lines.emplace_back(x1, y1, x2, y2, color, thickness);
	}

	void FlushLines(void)
	{
		for (const Line &line : lines)
			ImGui::GetForegroundDrawList()->AddLine(
				ImVec2(line.x1, line.y1),
				ImVec2(line.x2, line.y2),
				line.color,
				line.thickness
			);
		lines.clear();
	}
};

struct LinkedStringFunc
{
	const char *name;
	void (*func)(void);
};

#endif

#if 1 // region: private storage

static GuiSettings gGuiSettings;
static Scene *gScene;
static GuiInterop *gGui;

#endif

#if 1 // region: private functions

// imgui doesn't support this natively, so hack a custom one into it
static void MultiLineTabBarGeneric(
	const char *uniqueName
	, const void *arr[]
	, const int count
	, const int grouping
	, int *which
	, std::function<const char*(const void*[], int)> getter
)
{
	// here i do some hacky stuff with the ui theme colors so
	// multiple stacked tab bars appear to the user as a unified
	// tab bar spanning multiple lines
	
	ImGui::PushStyleVar(ImGuiStyleVar_TabBarBorderSize, 0.0f);
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;
	ImVec4 oldTabActive = colors[ImGuiCol_TabActive];
	ImVec4 oldTabHovered = colors[ImGuiCol_TabHovered];
	ImVec4 oldTab = colors[ImGuiCol_Tab];
	ImVec4 newTabHovered = ImVec4(0.7, 0, 0, 1.0); // TODO move elsewhere
	ImVec4 newTabActive = ImVec4(0.5, 0, 0, 1.0);
	int flags = ImGuiTabBarFlags_NoTooltip | ImGuiTabBarFlags_FittingPolicyResizeDown;
	
	colors[ImGuiCol_TabHovered] = newTabHovered;
	
	for (int i = 0; i < count; i += grouping)
	{
		static char name[1024];
		
		sprintf(name, "##%sRow%d", uniqueName, i);
		
		// test using buttons
		/*
		for (int k = 0; k < grouping && i + k < count; ++k)
		{
			int t = i + k;
			
			if (k > 0)
				ImGui::SameLine();
			
			ImGui::SmallButton(test[t]);
		}
		continue;
		*/
		
		if (i + grouping >= count)
		{
			colors[ImGuiCol_TabActive] = oldTab;
			colors[ImGuiCol_Tab] = oldTab;
			ImGui::PopStyleVar();
		}
		
		if (ImGui::BeginTabBar(name, flags))
		{
			for (int k = 0; k < grouping && i + k < count; ++k)
			{
				int t = i + k;
				
				if (t == *which)
				{
					colors[ImGuiCol_TabActive] = newTabActive;
					colors[ImGuiCol_Tab] = colors[ImGuiCol_TabActive];
				}
				else
				{
					colors[ImGuiCol_TabActive] = oldTab;
					colors[ImGuiCol_Tab] = oldTab;
				}
				
				const char *name = getter(arr, t);
				
				if (ImGui::BeginTabItem(name))
				{
					if (ImGui::IsItemDeactivated())
					{
						//fprintf(stderr, "selected %s\n", name);
						*which = t;
					}
					ImGui::EndTabItem();
				}
			}
			
			ImGui::EndTabBar();
		}
	}
	
	colors[ImGuiCol_TabActive] = oldTabActive;
	colors[ImGuiCol_TabHovered] = oldTabHovered;
	colors[ImGuiCol_Tab] = oldTab;
}

static void MultiLineTabBar(const char *uniqueName, char const *arr[], const int count, const int grouping, int *which)
{
	MultiLineTabBarGeneric(
		uniqueName
		, reinterpret_cast<const void**>(arr)
		, count
		, grouping
		, which
		, [](const void **arr, const int index) -> const char* {
			const char **ready = reinterpret_cast<const char**>(arr);
			return ready[index];
		}
	);
}

static void PickIntValue(const char *uniqueName, int *v, int min = INT_MIN, int max = INT_MAX)
{
	ImGuiIO &io = ImGui::GetIO();
	bool isSingleClick = false;
	
	// want single-click to behave as double-click so users click to edit
	if (io.MouseClickedCount[0] == 1)
	{
		io.MouseClickedCount[0] = 2;
		isSingleClick = true;
	}
	
	ImGui::DragInt(uniqueName, v, 0.00001f, 0, 0xffff, "0x%04x", ImGuiSliderFlags_None);
	
	if (isSingleClick)
		io.MouseClickedCount[0] = 1;
	
	*v = CLAMP(*v, min, max);
}

static void PickHexValueU16(const char *uniqueName, uint16_t *v, uint16_t min = 0, uint16_t max = 0xffff)
{
	int tmp = *v;
	
	PickIntValue(uniqueName, &tmp, min, max);
	
	*v = tmp;
}

static const LinkedStringFunc *gSidebarTabs[] = {
	new LinkedStringFunc{
		"General"
		, [](){
			ImGui::TextWrapped("TODO: 'General' tab");
		}
	},
	new LinkedStringFunc{
		"Rooms"
		, [](){
			ImGui::TextWrapped("TODO: 'Rooms' tab");
		}
	},
	new LinkedStringFunc{
		"Environment"
		, [](){
			ImGui::TextWrapped("TODO: 'Scene Env' tab");
			
			// test sb macro
			char test[16];
			sprintf(test, "%d light settings", sb_count(gScene->headers[0].lights));
			ImGui::TextWrapped(test);
			sprintf(test, "%p", &gScene->test);
			ImGui::TextWrapped(test);
			
			
			ImGui::SeparatorText("Environment Preview");
			ImGui::Checkbox("Fog##EnvironmentCheckbox", &gGui->env.isFogEnabled);
			ImGui::Checkbox("Lighting##EnvironmentCheckbox", &gGui->env.isLightingEnabled);
			const char *envPreviewModeNames[] = {
				"Each",
				"Time of Day"
			};
			ImGui::Combo(
				"##Environment##PreviewMode"
				, &gGui->env.envPreviewMode
				, envPreviewModeNames
				, GUI_ENV_PREVIEW_COUNT
			);
			IMGUI_COMBO_HOVER(gGui->env.envPreviewMode, GUI_ENV_PREVIEW_COUNT);
			switch (gGui->env.envPreviewMode)
			{
				case GUI_ENV_PREVIEW_EACH: {
					ImGui::Combo(
						"Index##EnvironmentEach"
						, &gGui->env.envPreviewEach
						, [](void* data, int n) {
							static char test[64];
							sprintf(test, "%d", n);
							return (const char*)test;
						}
						, gScene->headers[0].lights
						, sb_count(gScene->headers[0].lights)
					);
					IMGUI_COMBO_HOVER(gGui->env.envPreviewEach, sb_count(gScene->headers[0].lights));
					break;
				}
				
				case GUI_ENV_PREVIEW_TIME: {
					int time = gGui->env.envPreviewTime;
					ImGui::SliderInt("Drag to edit##EnvironmentTime", &time, 0, 0xffff, "0x%04x", ImGuiSliderFlags_AlwaysClamp);
					gGui->env.envPreviewTime = CLAMP(time, 0, 0xffff);
					break;
				}
			}
		}
	},
	new LinkedStringFunc{
		"Room Env"
		, [](){
			ImGui::TextWrapped("TODO: 'Room Env' tab");
		}
	},
	new LinkedStringFunc{
		"Collision & Exits"
		, [](){
			ImGui::TextWrapped("TODO: 'Collision & Exits' tab");
		}
	},
	new LinkedStringFunc{
		"Spawns"
		, [](){
			ImGui::TextWrapped("TODO: 'Spawns' tab");
		}
	},
	new LinkedStringFunc{
		"Pathways"
		, [](){
			ImGui::TextWrapped("TODO: 'Pathways' tab");
		}
	},
	new LinkedStringFunc{
		"Instances"
		, [](){
			RoomHeader *header = &gScene->rooms[0].headers[0];
			Instance *instances = header->instances;
			Instance *inst;
			
			if (sb_count(instances) == 0)
			{
				ImGui::TextWrapped("No instances. Add an instance to get started.");
				
				return;
			}
			
			ImGui::SeparatorText("Instance List");
			ImGui::Combo(
				"##Instance##InstanceCombo"
				, &gGuiSettings.combos.instanceCurrent
				, [](void* data, int n) {
					static char test[64];
					Instance *inst = &((Instance*)data)[n];
					sprintf(test, "%d: 0x%04x", n, inst->id);
					return (const char*)test;
				}
				, instances
				, sb_count(instances)
			);
			IMGUI_COMBO_HOVER(gGuiSettings.combos.instanceCurrent, sb_count(instances));
			inst = &instances[gGuiSettings.combos.instanceCurrent];
			ImGui::Button("Add##InstanceCombo"); ImGui::SameLine();
			ImGui::Button("Duplicate##InstanceCombo"); ImGui::SameLine();
			ImGui::Button("Delete##InstanceCombo");
			
			ImGui::SeparatorText("Data");
			
			// id w/ search
			PickHexValueU16("ID##Instance", &inst->id);
			ImGui::SameLine();
			if (ImGui::Button("Search##Instance"))
				ImGui::OpenPopup("InstanceTypeSearch");
			if (ImGui::BeginPopup("InstanceTypeSearch"))
			{
				static ImGuiTextFilter filter;
				filter.Draw();
				
				const char* names[] = {
					"Stalfos"
					, "Stalchild"
					, "Stalhorse"
					, "Stalhound"
					, "Lizalfos"
					, "Moblin"
					, "Deku Baba"
					, "Deku Scrub"
					, "Deku Tree"
					, "Mad Scrub"
				};
				
				for (int i = 0; i < IM_ARRAYSIZE(names); i++)
					if (filter.PassFilter(names[i]))
						if (ImGui::Selectable(names[i]))
							inst->id = i;
				ImGui::EndPopup();
			}
			
			// params
			PickHexValueU16("Params##Instance", &inst->params);
			
			ImGui::SeparatorText("Position");
			int xpos = inst->x;
			int ypos = inst->y;
			int zpos = inst->z;
			ImGui::InputInt("X##InstancePos", &xpos);
			ImGui::InputInt("Y##InstancePos", &ypos);
			ImGui::InputInt("Z##InstancePos", &zpos);
			inst->x = xpos;
			inst->y = ypos;
			inst->z = zpos;
			
			ImGui::SeparatorText("Rotation");
			int xrot = inst->xrot;
			int yrot = inst->yrot;
			int zrot = inst->zrot;
			ImGui::InputInt("X##InstanceRot", &xrot);
			ImGui::InputInt("Y##InstanceRot", &yrot);
			ImGui::InputInt("Z##InstanceRot", &zrot);
			inst->xrot = xrot;
			inst->yrot = yrot;
			inst->zrot = zrot;
			
			ImGui::SeparatorText("Options");
			static int item_current_idx = 0; // Here we store our selection data as an index.
			const char* items[] = { "TODO", "source", "from", "config" };
			if (ImGui::BeginListBox("##Options", ImVec2(-FLT_MIN, 5 * ImGui::GetTextLineHeightWithSpacing())))
			{
				for (int n = 0; n < 10; n++)
				{
					const bool is_selected = (item_current_idx == n);
					if (ImGui::Selectable(items[n % IM_ARRAYSIZE(items)], is_selected))
						item_current_idx = n;

					// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndListBox();
			}
			// TODO different value input types for different options
			static int todoOptions; ImGui::InputInt("Value##InstanceOptions", &todoOptions);
		}
	},
	new LinkedStringFunc{
		"Objects"
		, [](){
			ImGui::TextWrapped("TODO: 'Objects' tab");
		}
	},
};

static void DrawSidebar(void)
{
	ImGuiIO& io = ImGui::GetIO();
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoSavedSettings
		| ImGuiWindowFlags_NoFocusOnAppearing
		| ImGuiWindowFlags_NoNav
		| ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoBringToFrontOnFocus
		| ImGuiWindowFlags_NoMove
	;
	
	// positioning
	{
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		const int minWidth = 256;
		int maxWidth = -1;
		
		ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
		ImVec2 work_size = viewport->WorkSize;
		ImVec2 window_pos, window_pos_pivot;
		
		maxWidth = work_size.x / 2;
		if (maxWidth < minWidth)
			maxWidth = minWidth;
		
		window_pos.x = work_pos.x + work_size.x;
		window_pos.y = work_pos.y;
		window_pos_pivot.x = 1;
		window_pos_pivot.y = 0;
		
		ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
		ImGui::SetNextWindowSizeConstraints(ImVec2(minWidth, work_size.y), ImVec2(maxWidth, work_size.y));
		ImGui::SetNextWindowBgAlpha(0.35f); // translucent background
	}
	
	if (ImGui::Begin("Sidebar", 0, window_flags))
	{
		static int which = 0;
		
		MultiLineTabBarGeneric(
			"SidebarTabs"
			, reinterpret_cast<const void**>(gSidebarTabs)
			, sizeof(gSidebarTabs) / sizeof(*gSidebarTabs)
			, 5
			, &which
			, [](const void **arr, const int index) -> const char* {
				const LinkedStringFunc **ready = reinterpret_cast<const LinkedStringFunc**>(arr);
				return ready[index]->name;
			}
		);
		//ImGui::TextWrapped(gSidebarTabs[which]->name); // test
		
		// draw the selected sidebar
		if (gScene)
			gSidebarTabs[which]->func();
		
		// example using MultiLineTabBar(), for illustrative purposes
		/*
		static char const *test[] = {
			"General", "Rooms", "Scene Env", "Room Env",
			"Collision & Exits", "Spawns", "Pathways", "Instances", "Objects"
		};
		MultiLineTabBar("SidebarTabsExample", test, sizeof(test) / sizeof(*test), 5, &which);
		ImGui::TextWrapped(test[which]);
		*/
		
		// maybe use vertical tabs w/ icons and tooltips eventually (proof of concept)
		/*
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::PushClipRect((ImVec2){0, 0}, viewport->Size, false);
		for (int i = 0; i < sizeof(test) / sizeof(*test); ++i)
		{
			ImGui::SetCursorPos((ImVec2){-64, i * 32.0f});
			ImGui::Button(test[i]);
		}
		ImGui::PopClipRect();
		*/
	}
	ImGui::End();
}

static void DrawMenuBar(void)
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New"))
			{
			}
			if (ImGui::MenuItem("Open", "Ctrl+O"))
			{
				fprintf(stderr, "wow!\n");
				
				Scene *newScene = WindowOpenFile();
				
				if (newScene && newScene != gScene)
				{
					gGuiSettings.Reset();
					gScene = newScene;
				}
			}
			if (ImGui::MenuItem("Save", "Ctrl+S"))
			{
			}
			if (ImGui::MenuItem("Save as..."))
			{
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Exit", "Ctrl+Q"))
			{
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("View"))
		{
			if (ImGui::MenuItem("Sidebar", NULL, &gGuiSettings.showSidebar));
			#ifndef NDEBUG
			ImGui::MenuItem("ImGui Demo Window", NULL, &gGuiSettings.showImGuiDemoWindow);
			#endif
			
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}
#endif // end private functions

extern "C" void GuiInit(GLFWwindow *window)
{
	// Decide GL+GLSL versions
	#if defined(IMGUI_IMPL_OPENGL_ES2)
	// GL ES 2.0 + GLSL 100
	const char* glsl_version = "#version 100";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	#elif defined(__APPLE__)
	// GL 3.2 + GLSL 150
	const char* glsl_version = "#version 150";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
	#else
	// GL 3.0 + GLSL 130
	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
	#endif
	
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);
}

extern "C" void GuiCleanup(void)
{
	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

extern "C" void GuiDraw(GLFWwindow *window, struct Scene *scene, struct GuiInterop *interop)
{
	// saves us the trouble of passing this variable around
	gScene = scene;
	gGui = interop;
	
	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	
	// lines
	gGuiSettings.FlushLines();
	
	DrawMenuBar();
	
	if (gGuiSettings.showSidebar)
		DrawSidebar();
	
	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	if (gGuiSettings.showImGuiDemoWindow)
		ImGui::ShowDemoWindow(&gGuiSettings.showImGuiDemoWindow);
	
	// Rendering
	ImGui::Render();
	int display_w, display_h;
	glfwGetFramebufferSize(window, &display_w, &display_h);
	
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

extern "C" int GuiHasFocus(void)
{
	return ImGui::GetIO().WantCaptureMouse
		|| ImGui::GetIO().WantCaptureKeyboard
	;
}

extern "C" void GuiPushLine(int x1, int y1, int x2, int y2, uint32_t color, float thickness)
{
	// byteswap
	uint32_t newcolor = 0;
	newcolor |= ((color >> 0) & 0xff) << 24;
	newcolor |= ((color >> 8) & 0xff) << 16;
	newcolor |= ((color >> 16) & 0xff) << 8;
	newcolor |= ((color >> 24) & 0xff) << 0;
	gGuiSettings.PushLine(x1, y1, x2, y2, newcolor, thickness);
}

