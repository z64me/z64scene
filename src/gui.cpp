#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <functional>
#include <climits>
#include "misc.h"
#include "gui.h"
#include "toml-parsers.hpp"

extern "C" {
#include <n64texconv.h>
#include <stb_image.h>
#include <stb_image_write.h>
#include "noc_file_dialog.h"
#include "project.h"
#include "logging.h"
#include "window.h"
}

#if 1 // region: helper macros

#define CLAMP(VALUE, MIN, MAX) (((VALUE) < (MIN)) ? (MIN) : ((VALUE) > (MAX)) ? (MAX) : (VALUE))
#define WRAP(VALUE, MIN, MAX) (((VALUE) < (MIN)) ? (MAX) : ((VALUE) > (MAX)) ? (MIN) : (VALUE))

// allows using mousewheel while hovering the last-drawn combo box to quickly change its value
#define IMGUI_COMBO_HOVER_ONCHANGE(CURRENT, HOWMANY, ONCHANGE) \
	if (ImGui::IsItemHovered() && ImGui::GetIO().MouseWheel) \
	{ \
		CURRENT = WRAP((CURRENT) - ImGui::GetIO().MouseWheel, 0, (HOWMANY) - 1); \
		do ONCHANGE while(0); \
	}
#define IMGUI_COMBO_HOVER(CURRENT, HOWMANY) IMGUI_COMBO_HOVER_ONCHANGE(CURRENT, HOWMANY, {})

#define HELP_SEPARATOR(TITLE, HELPTEXT) \
	ImGui::SeparatorText(TITLE "   "); \
	ImGui::SameLine(ImGui::CalcTextSize(TITLE).x + 32); \
	HelpMarker(HELPTEXT);

#endif

#if 1 // region: misc decls

// sidebar tabs
#define TABNAME_ACTORS "Actors"
#define TABNAME_DOORS "Doorways"
#define TABNAME_SPAWNS "Spawns"

#define DECL_POPUP(NAME) static bool isPopup##NAME##Queued = false;
#define QUEUE_POPUP(NAME) isPopup##NAME##Queued = true;
#define DEQUEUE_POPUP(NAME) if (isPopup##NAME##Queued) { ImGui::OpenPopup(#NAME); isPopup##NAME##Queued = false; }
#define STRINGIFY(NAME) #NAME

DECL_POPUP(AddNewInstanceSearch)
static enum InstanceTab gAddNewInstanceSearchTab = INSTANCE_TAB_ACTOR;

DECL_POPUP(AddNewObjectSearch)

#define GUI_ERROR_POPUP_ID "Error##GuiMainErrorMessage"

#endif

#if 1 // region: private types

struct GuiSettings
{
	bool showSidebar = true;
	bool showImGuiDemoWindow = false;
	
	ActorDatabase actorDatabase;
	ObjectDatabase objectDatabase;
	Project *project;
	const char *errorMessagePopupBody;
	bool projectIsReady;
	
	// combo boxes can be volatile as sizes change
	// when loading different scenes for editing
	struct
	{
		struct TextureBlob *textureBlob;
		int textureBlobIndex;
	} combos;
	
	void Reset(void)
	{
		memset(&combos, 0, sizeof(combos));
	}

private:
	// small status messages that appear in the corner and fade out
	struct Modal
	{
		float timeoutStart;
		float timeout;
		char message[256];
		bool hasDrawn = false;
		
		Modal(const char *message, float timeout = 5)
		{
			this->timeoutStart = timeout;
			this->timeout = timeout;
			this->message[0] = '\0';
			strncat(this->message, message, sizeof(this->message) - 1);
		}
	};
	std::vector<Modal> modals;

public:
	void DrawModals(void)
	{
		ImGuiWindowFlags window_flags =
			ImGuiWindowFlags_NoDecoration
			| ImGuiWindowFlags_AlwaysAutoResize
			| ImGuiWindowFlags_NoSavedSettings
			| ImGuiWindowFlags_NoFocusOnAppearing
			| ImGuiWindowFlags_NoNav
			| ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoMouseInputs
		;
		const float timeoutFadeStart = 0.5; // start fadeout when timeout reaches this value
		const float PAD = 10.0f;
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGuiIO& io = ImGui::GetIO();
		const int location = 2; // lower left corner
		ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
		ImVec2 work_size = viewport->WorkSize;
		ImVec2 window_pos, window_pos_pivot;
		window_pos.x = (location & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
		window_pos.y = (location & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
		window_pos_pivot.x = (location & 1) ? 1.0f : 0.0f;
		window_pos_pivot.y = (location & 2) ? 1.0f : 0.0f;
		char unique[32];
		int i = 0;
		
		for (Modal &modal : modals)
		{
			ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
			float windowOpacity = 0.35f;
			float textOpacity = 1.0f;
			ImVec2 windowSize = {0, 0};
			
			// setup fadeout
			if (timeoutFadeStart && modal.timeout <= timeoutFadeStart)
			{
				float mul = modal.timeout / timeoutFadeStart;
				
				windowOpacity *= mul;
				textOpacity *= mul;
			}
			
			// update style
			ImGuiStyle* style = &ImGui::GetStyle();
			ImGui::SetNextWindowBgAlpha(windowOpacity);
			float textOpaOld = style->Colors[ImGuiCol_Text].w;
			float borderOpaOld = style->Colors[ImGuiCol_Border].w;
			style->Colors[ImGuiCol_Text].w *= textOpacity;
			style->Colors[ImGuiCol_Border].w *= textOpacity;
			
			// draw
			sprintf(unique, "##modal%d\n", ++i);
			if (ImGui::Begin(unique, 0, window_flags))
			{
				ImGui::Text(modal.message);
				windowSize = ImGui::GetWindowSize();
			}
			ImGui::End();
			
			// restore style
			style->Colors[ImGuiCol_Text].w = textOpaOld;
			style->Colors[ImGuiCol_Border].w = borderOpaOld;
			
			// advance
			if (modal.hasDrawn)
				modal.timeout -= io.DeltaTime;
			window_pos.y -= windowSize.y + PAD;
			modal.hasDrawn = true;
		}
		
		while (!modals.empty() && modals.back().timeout <= 0)
			modals.pop_back();
	}
	
	void PushModal(const char *message, float timeout = 2)
	{
		modals.insert(modals.begin(), Modal(message, timeout));
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

std::map<std::string, uint32_t> GetObjectSymbolAddresses(uint16_t objId)
{
	return gGuiSettings.objectDatabase.GetEntry(objId).symbolAddresses;
}

sb_find_impl(ObjectListContains, ObjectEntry, uint16_t, each->id == needle)

static void TestAllActorRenderCodeGen(void)
{
	for (auto &entry : gGuiSettings.actorDatabase.entries)
	{
		if (entry.isEmpty)
			continue;
		
		const char *tmp = entry.RenderCodeGen(GetObjectSymbolAddresses);
		if (tmp)
		{
			LogDebug(
				"\n-------- codegen --------\n"
				"tomlLineStart = %d\n"
				"offsetLineStart = %d\n"
				"%s\n"
				"---------- end ----------"
				, entry.rendercodeLineNumberInToml
				, entry.rendercodeLineNumberOffset
				, tmp
			);
		}
	}
}

// Helper to display a little (?) mark which shows a tooltip when hovered.
// In your own code you may want to display an actual icon if you are using a merged icon fonts (see docs/FONTS.md)
static void HelpMarker(const char *desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::BeginItemTooltip())
	{
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

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
						//LogDebug("selected %s", name);
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

static bool PickColor3U8(const char* label, uint8_t *rgb, ImGuiColorEditFlags flags = 0)
{
	float col[3] = { UNFOLD_ARRAY_3_EXT(uint8_t, rgb, / 255.0f) };
	bool result = ImGui::ColorEdit3(label, col, flags);
	
	for (int i = 0; i < 3; ++i)
		rgb[i] = col[i] * 255;
	
	return result;
}

static bool PickColor3U8(const char* label, ZeldaRGB *rgb, ImGuiColorEditFlags flags = 0)
{
	return PickColor3U8(label, (uint8_t*)rgb, flags);
}

static bool PickColor3U8(const char* label, ZeldaVecS8 *rgb, ImGuiColorEditFlags flags = 0)
{
	return PickColor3U8(label, (uint8_t*)rgb, flags);
}

// determine which objects are missing, and which are potentially unused
static void RefreshObjectStats(void)
{
	sb_array(ObjectEntry, haveObjects) = *(gGui->objectList);
	sb_array(ObjectEntry, missingObjects) = gGui->missingObjects;
	sb_array(ObjectEntry, unusedObjects) = gGui->unusedObjects;
	
	sb_clear(missingObjects);
	sb_clear(unusedObjects);
	
	LogDebug("RefreshObjectStats()");
	
	sb_foreach(haveObjects, {
		each->children = 0;
	})
	gGui->subkeepChildren = 0;
	
	for (int i = 0; i < 2; ++i)
	{
		sb_array(Instance, list);
		
		switch (i) {
			case 0: list = *(gGui->actorList); break;
			case 1: list = *(gGui->doorList); break;
		}
		
		sb_foreach_named(list, actor, {
			auto &type = gGuiSettings.actorDatabase.GetEntry(actor->id);
			
			if (type.isEmpty)
				continue;
			
			for (auto needObject : type.objects) {
				if (needObject <= 0x0001) // filter global obj dep
					continue;
				if (gGui->sceneHeader->specialFiles // filter subkeep obj deps
					&& needObject == gGui->sceneHeader->specialFiles->subkeepObjectId
				) {
					gGui->subkeepChildren += 1;
					continue;
				}
				ObjectEntry *used = ObjectListContains(haveObjects, needObject);
				if (used) {
					used->children += 1;
					continue;
				}
				ObjectEntry *missing = ObjectListContains(missingObjects, needObject);
				if (!missing)
					missing = &sb_push(missingObjects, ((ObjectEntry){
						.id = needObject,
						.type = OBJECT_ENTRY_TYPE_IMPLIED,
					}));
				missing->children += 1;
			}
		})
	}
	
	sb_foreach(haveObjects, {
		if (each->children == 0)
			sb_push(unusedObjects, *each);
	})
	
	gGui->unusedObjects = unusedObjects;
	gGui->missingObjects = missingObjects;
}

static ActorDatabase::Entry &InstanceTypeSearch(void)
{
	static ImGuiTextFilter filter;
	filter.Draw();
	
	for (auto &entry : gGuiSettings.actorDatabase.entries)
	{
		// skip blank/deleted entries
		if (entry.isEmpty)
			continue;
		
		const char *name = entry.name;
		
		// custom filters
		switch (gAddNewInstanceSearchTab)
		{
			case INSTANCE_TAB_ACTOR:
				if (entry.categoryInt == ACTORCAT_PLAYER
					|| entry.categoryInt == ACTORCAT_DOOR
				)
					continue;
				break;
			
			case INSTANCE_TAB_SPAWN:
				if (entry.categoryInt != ACTORCAT_PLAYER)
					continue;
				break;
			
			case INSTANCE_TAB_DOOR:
				if (entry.categoryInt != ACTORCAT_DOOR)
					continue;
				break;
			
			default:
				Die("unknown tab type");
				break;
		}
		
		if (name
			&& filter.PassFilter(name)
			&& ImGui::Selectable(name)
		)
			return entry;
	}
	
	static ActorDatabase::Entry empty;
	empty.isEmpty = true;
	
	return empty;
}

static ObjectDatabase::Entry &ObjectTypeSearch(void)
{
	static ImGuiTextFilter filter;
	filter.Draw();
	
	for (auto &entry : gGuiSettings.objectDatabase.entries)
	{
		// skip blank/deleted entries
		if (entry.isEmpty)
			continue;
		
		// don't allow the first few objects
		if (entry.index <= 3)
			continue;
		
		const char *name = entry.name;
		
		if (name
			&& filter.PassFilter(name)
			&& ImGui::Selectable(name)
		)
			return entry;
	}
	
	static ObjectDatabase::Entry empty;
	empty.isEmpty = true;
	
	return empty;
}

// multi-purpose instance tab, good for actors/spawnpoints/doors
// XXX if "String##Instance" etc give issues, try QuickFmt("String##%s", tabType)
static Instance *InstanceTab(sb_array(struct Instance, *instanceList), const char *tabType, enum InstanceTab tab)
{
	gGui->instanceList = instanceList;
	Instance *instances = *(gGui->instanceList);
	
	if (ImGui::Button(QuickFmt("Add New %s##InstanceCombo", tabType)))
	{
		QUEUE_POPUP(AddNewInstanceSearch);
		gAddNewInstanceSearchTab = tab;
	}
	
	Instance *inst = gGui->selectedInstance;
	int instanceCurIndex = sb_find_index(instances, inst);
	bool instanceSelectionChanged = false;
	
	if (sb_count(instances) == 0)
	{
		ImGui::TextWrapped("No instances. Add an instance to get started.");
		
		return 0;
	}
	
	ImGui::SeparatorText(QuickFmt("%s List", tabType));
	// new code, with filtering by day
	char previewText[256];
	if (instanceCurIndex >= 0)
		snprintf(previewText, sizeof(previewText), "%d: %s 0x%04x"
			, instanceCurIndex
			, gGuiSettings.actorDatabase.GetActorName(inst->id)
			, inst->id
		);
	else
		strcpy(previewText, "");
	if (ImGui::BeginCombo("##Instance##InstanceCombo", previewText, 0))
	{
		int count = sb_count(instances);
		uint16_t guiHalfDayBits = gGui->halfDayBits;
		
		for (int i = 0; i < count; ++i)
		{
			Instance *inst = instances + i;
			
			if (!(inst->mm.halfDayBits & guiHalfDayBits))
				continue;
			
			const bool isSelected = (gGui->selectedInstance - instances == i);
			// TODO DRY
			snprintf(previewText, sizeof(previewText), "%d: %s 0x%04x"
				, i
				, gGuiSettings.actorDatabase.GetActorName(inst->id)
				, inst->id
			);
			
			if (ImGui::Selectable(previewText, isSelected))
				gGui->selectedInstance = inst;

			// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
			if (isSelected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	// on mousewheel, select next actor with matching halfDayBits
	for (bool keepLooping = instanceCurIndex >= 0; keepLooping; ) {
		IMGUI_COMBO_HOVER_ONCHANGE(instanceCurIndex, sb_count(instances), {
			if ((instances[instanceCurIndex].mm.halfDayBits & gGui->halfDayBits))
				keepLooping = false;
		}) else
			break;
	}
	// old code, for reference
	/*
	ImGui::Combo(
		"##Instance##InstanceCombo"
		, &instanceCurIndex
		, [](void* data, int n) {
			static char test[256];
			// possible safety, but i think imgui handles it automatically
			//if (n < 0)
			//	return (const char*)strcpy(test, "");
			Instance *inst = &((Instance*)data)[n];
			sprintf(test, "%d: %s 0x%04x", n, gGuiSettings.actorDatabase.GetActorName(inst->id), inst->id);
			return (const char*)test;
		}
		, instances
		, sb_count(instances)
	);
	IMGUI_COMBO_HOVER(instanceCurIndex, sb_count(instances));
	*/
	
	inst = &instances[instanceCurIndex];
	if (instanceCurIndex < 0) inst = 0;
	
	// different instance selected than last time, reset some things
	ON_CHANGE(inst)
	{
		instanceSelectionChanged = true;
		gGui->selectedInstance = inst;
	}
	
	if (instanceCurIndex < 0)
	{
		ImGui::TextWrapped("No instance selected.");
		
		return 0;
	}
	
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
		auto &entry = InstanceTypeSearch();
		
		if (!entry.isEmpty)
			inst->id = entry.index;
		
		ImGui::EndPopup();
	}
	
	// params
	PickHexValueU16("Params##Instance", &inst->params);
	
	ImGui::SeparatorText("Position");
	int xpos = rintf(inst->pos.x);
	int ypos = rintf(inst->pos.y);
	int zpos = rintf(inst->pos.z);
	int nudgeBy = (ImGui::GetIO().KeyShift) ? 20 : 1; // nudge by 20 if shift is held
	int nudgeCtrl = nudgeBy; // ctrl has no effect
	if (ImGui::InputInt("X##InstancePos", &xpos, nudgeBy, nudgeCtrl)) inst->pos.x = xpos;
	ImGui::SameLine(); HelpMarker("These buttons nudge farther\n""if you hold the Shift key.");
	if (ImGui::InputInt("Y##InstancePos", &ypos, nudgeBy, nudgeCtrl)) inst->pos.y = ypos;
	if (ImGui::InputInt("Z##InstancePos", &zpos, nudgeBy, nudgeCtrl)) inst->pos.z = zpos;
	
	ImGui::SeparatorText("Rotation");
	int xrot = inst->xrot;
	int yrot = inst->yrot;
	int zrot = inst->zrot;
	nudgeBy = (ImGui::GetIO().KeyShift) ? 0x2000 : 1; // nudge by 20 if shift is held
	nudgeCtrl = 0xffff / 360; // 1 degree
	if (tab != INSTANCE_TAB_DOOR) ImGui::InputInt("X##InstanceRot", &xrot, nudgeBy, nudgeCtrl);
	ImGui::InputInt("Y##InstanceRot", &yrot, nudgeBy, nudgeCtrl);
	ImGui::SameLine(); HelpMarker(
		"These buttons turn by 45 degrees if you hold the\n"
		"Shift key, or by 1 degree if you hold the Ctrl key."
	);
	if (tab != INSTANCE_TAB_DOOR) ImGui::InputInt("Z##InstanceRot", &zrot, nudgeBy, nudgeCtrl);
	inst->xrot = xrot;
	inst->yrot = yrot;
	inst->zrot = zrot;
	
	auto &options = gGuiSettings.actorDatabase.GetEntry(inst->id).properties;
	if (options.size())
	{
		ImGui::SeparatorText("Options");
		
		static ActorDatabase::Entry::Property *current = 0;
		
		// when a new instance is selected, clear the selected property
		if (instanceSelectionChanged)
			current = 0;
		
		if (ImGui::BeginListBox("##Options", ImVec2(-FLT_MIN, 5 * ImGui::GetTextLineHeightWithSpacing())))
		{
			for (auto &option : options)
			{
				const bool is_selected = (current == &option);
				
				if (ImGui::Selectable(option.name, is_selected))
					current = &option;

				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndListBox();
		}
		
		if (current)
		{
			int comboOpt = current->Extract(inst->params, inst->xrot, inst->yrot, inst->zrot);
			
			// TODO different value input types for different options? or hide this if combos.size()?
			ImGui::InputInt("Value##Instance##InstanceOptions##Value", &comboOpt);
			
			// display combo boxes
			auto &combos = current->combos;
			if (combos.size())
			{
				const char *previewText = "Custom Value (Above)";
				int selectedIndex = -1;
				int i;
				
				i = 0;
				for (auto &combo : combos)
				{
					if (combo.value == comboOpt)
					{
						previewText = combo.label;
						selectedIndex = i;
					}
					++i;
				}
				
				if (ImGui::BeginCombo("##Instance##InstanceOptions##Dropdown", previewText))
				{
					i = 0;
					for (auto &combo : combos)
					{
						if (ImGui::Selectable(combo.label, combo.value == comboOpt))
						{
							comboOpt = combo.value;
							selectedIndex = i;
						}
					}
					ImGui::EndCombo();
				}
				IMGUI_COMBO_HOVER_ONCHANGE(selectedIndex, combos.size(), { comboOpt = combos[selectedIndex].value; });
			}
			
			current->Inject(comboOpt, &inst->params, &inst->xrot, &inst->yrot, &inst->zrot);
		}
	}
	
	return inst;
}

static void ObjectTabList(const char *tag, sb_array(ObjectEntry, list), ObjectEntry **current_)
{
	if (!current_) return;
	struct ObjectEntry *current = *current_;
	if (ImGui::BeginListBox(tag, ImVec2(-FLT_MIN, 5 * ImGui::GetTextLineHeightWithSpacing())))
	{
		if (sb_count(list) > 0)
			sb_foreach(list, {
				const bool is_selected = (current == each);
				
				if (each->name == 0)
					each->name = gGuiSettings.objectDatabase.GetObjectName(each->id);
				
				if (ImGui::Selectable(each->name, is_selected))
					current = each;
				
				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			})
		ImGui::EndListBox();
	}
	*current_ = current;
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
		TABNAME_DOORS
		, [](){
			
			Instance *inst = InstanceTab(gGui->doorList, "Doorway", INSTANCE_TAB_DOOR);
			
			if (!inst)
				return;
			
			// room change data
			ImGui::SeparatorText("Room Change Data");
			
			int frontRoom = inst->doorway.frontRoom;
			int frontCamera = inst->doorway.frontCamera;
			int backRoom = inst->doorway.backRoom;
			int backCamera = inst->doorway.backCamera;
			
			// XXX swapped front/back to compensate for default rotation
			if (ImGui::InputInt("Front Room##DoorwayData", &backRoom)) inst->doorway.backRoom = backRoom;
			if (ImGui::InputInt("Front Camera##DoorwayData", &backCamera)) inst->doorway.backCamera = backCamera;
			if (ImGui::InputInt("Back Room##DoorwayData", &frontRoom)) inst->doorway.frontRoom = frontRoom;
			if (ImGui::InputInt("Back Camera##DoorwayData", &frontCamera)) inst->doorway.frontCamera = frontCamera;
		}
	},
	new LinkedStringFunc{
		"Environment"
		, [](){
			ImGui::TextWrapped("TODO: 'Scene Env' tab");
			
			// test sb macro
			char test[16];
			int numLights = sb_count(gGui->sceneHeader->lights);
			sprintf(test, "%d light settings", numLights);
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
							sprintf(test, "Env %d", n);
							return (const char*)test;
						}
						, gGui->sceneHeader->lights
						, numLights
					);
					IMGUI_COMBO_HOVER(gGui->env.envPreviewEach, numLights);
					{
						ZeldaLight *light = &gGui->sceneHeader->lights[gGui->env.envPreviewEach];
						
						// TODO using color to represent direction vector for now
						ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoInputs;
						PickColor3U8("Ambient Light##EnvEditor", &light->ambient, flags);
						PickColor3U8("Diffuse A##EnvEditor", &light->diffuse_a, flags);
							ImGui::SameLine(); PickColor3U8("Direction##DiffA##EnvEditor", &light->diffuse_a_dir, flags);
						PickColor3U8("Diffuse B##EnvEditor", &light->diffuse_b, flags);
							ImGui::SameLine(); PickColor3U8("Direction##DiffB##EnvEditor", &light->diffuse_b_dir, flags);
						PickColor3U8("Fog##EnvEditor", &light->fog, flags);
					}
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
		TABNAME_SPAWNS
		, [](){
			
			Instance *inst = InstanceTab(gGui->spawnList, "Spawn Point", INSTANCE_TAB_SPAWN);
			
			if (!inst)
				return;
			
			int room = inst->spawnpoint.room;
			
			if (ImGui::InputInt("Room##SpawnPointData", &room)) inst->spawnpoint.room = room;
		}
	},
	new LinkedStringFunc{
		"Pathways"
		, [](){
			ImGui::TextWrapped("TODO: 'Pathways' tab");
		}
	},
	new LinkedStringFunc{
		TABNAME_ACTORS
		, [](){
			
			InstanceTab(gGui->actorList, "Actor", INSTANCE_TAB_ACTOR);
		}
	},
	new LinkedStringFunc{
		"Objects"
		, [](){
			ImGui::TextWrapped("TODO: 'Objects' tab");
			// the commented code is for buttons along the side of the list box
			
			if (gGui->sceneHeader->specialFiles)
			{
				const char *preview = "Unknown";
				static struct {
					const char *name;
					uint16_t objId;
				} ok[] = {
					{ "None", 0x0000 },
					{ "Field", 0x0002 },
					{ "Dungeon", 0x0003 },
				};
				int subkeep = gGui->sceneHeader->specialFiles->subkeepObjectId;
				int newSubkeep = subkeep;
				
				for (int i = 0; i < sizeof(ok) / sizeof(*ok); ++i)
					if (subkeep == ok[i].objId)
						preview = ok[i].name;
				
				ImGui::Text("Subkeep Object:"); ImGui::SameLine(); ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::BeginCombo("##Subkeep Object", preview))
				{
					for (int i = 0; i < sizeof(ok) / sizeof(*ok); ++i)
						if (ImGui::Selectable(ok[i].name, subkeep == ok[i].objId))
							newSubkeep = ok[i].objId;
					ImGui::EndCombo();
				}
				if (subkeep)
					ImGui::TextWrapped("'%s' is needed by %d actors", preview, gGui->subkeepChildren);
				
				gGui->sceneHeader->specialFiles->subkeepObjectId = newSubkeep;
			}
			
			struct ObjectEntry *current = gGui->selectedObject;
			struct ObjectEntry *list = *gGui->objectList;
			
			ObjectTabList("##ObjectListMain", list, &current);
			//ImGui::SameLine();
			//ImVec2 nextPos = ImGui::GetCursorPos();
			//int yadv = ImGui::GetTextLineHeightWithSpacing() + 5; // padding
			if (ImGui::ArrowButton("Raise##ObjectList", ImGuiDir_Up) && current > list)
			{
				struct ObjectEntry tmp = current[-1];
				current[-1] = current[0];
				current[0] = tmp;
				current -= 1;
			}
			ImGui::SameLine();
			if (ImGui::ArrowButton("Lower##ObjectList", ImGuiDir_Down) && current != &sb_last(list))
			{
				struct ObjectEntry tmp = current[1];
				current[1] = current[0];
				current[0] = tmp;
				current += 1;
			}
			ImGui::SameLine();
			
			//nextPos.y += yadv;
			//ImGui::SetCursorPos(nextPos);
			if (ImGui::Button("Add Object##ObjectList"))
			{
				QUEUE_POPUP(AddNewObjectSearch);
			}
			ImGui::SameLine();
			
			//nextPos.y += yadv;
			//ImGui::SetCursorPos(nextPos);
			if (ImGui::Button("Delete##ObjectList") && current >= list && current <= &sb_last(list))
			{
				sb_remove(list, current - list);
				
				if (current > &sb_last(list))
					current -= 1;
				
				if (sb_count(list) == 0)
					current = 0;
			}
			
			if (current)
			{
				ImGui::TextWrapped(current->name);
				ImGui::TextWrapped("Needed by %d actors", current->children);
			}
			
			gGui->selectedObject = current;
			
			// missing objects
			if (sb_count(gGui->missingObjects))
			{
				static ObjectEntry *current = 0;
				
				// on any reallocations etc
				ON_CHANGE(gGui->missingObjects) current = 0;
				
				HELP_SEPARATOR("Missing Objects",
					"These objects may be needed by\n"
					"certain actors you have placed,\n"
					"but are not present."
				);
				
				ObjectTabList("##ObjectListMissing", gGui->missingObjects, &current);
				
				if (current)
				{
					ImGui::TextWrapped("Needed by %d actors", current->children);
					
					if (current->id <= 3)
					{
						if (ImGui::Button("Set Subkeep Object"))
						{
							if (gGui->sceneHeader->specialFiles)
								gGui->sceneHeader->specialFiles->subkeepObjectId = current->id;
						}
					}
					else if (ImGui::Button("Add to Object List"))
					{
						ObjectEntry *missingObjects = gGui->missingObjects;
						gGui->selectedObject = &sb_push(*gGui->objectList, *current);
						sb_remove(missingObjects, current - missingObjects);
						if (current > &sb_last(missingObjects))
							current -= 1;
						if (sb_count(missingObjects) == 0)
							current = 0;
					}
				}
			}
			
			// potentially unused objects
			if (sb_count(gGui->unusedObjects))
			{
				static ObjectEntry *current = 0;
				
				// on any reallocations etc
				ON_CHANGE(gGui->unusedObjects) current = 0;
				
				HELP_SEPARATOR("Unused Objects",
					"These objects are potentially unused.\n"
					"They are not directly referenced by any\n"
					"of the actors present in the actor list."
				);
				
				ObjectTabList("##ObjectListUnused", gGui->unusedObjects, &current);
				
				if (current)
				{
					if (ImGui::Button("Delete from Object List"))
					{
						ObjectEntry *unusedObjects = gGui->missingObjects;
						ObjectEntry *objectList = *gGui->objectList;
						ObjectEntry *match = ObjectListContains(objectList, current->id);
						if (match) {
							sb_remove(objectList, match - objectList);
							gGui->selectedObject = 0;
						}
						sb_remove(unusedObjects, current - unusedObjects);
						if (current > &sb_last(unusedObjects))
							current -= 1;
						if (sb_count(unusedObjects) == 0)
							current = 0;
					}
				}
			}
		}
	},
	new LinkedStringFunc{
		"Textures"
		, [](){
			static int imageWidth = 0;
			static int imageHeight = 0;
			static uint8_t *imageData = 0;
			static int imageFmt;
			static int imageSiz;
			static GLuint imageTexture;
			static bool reloadTexture = false;
			static bool isBadTexture = false;
			static uint32_t segAddr = 0;
			static uint32_t palAddr = 0;
			static int scale = 0;
			const int scaleMax = 3;
			
			if (!sb_count(gScene->textureBlobs))
			{
				ImGui::Text("No textures detected");
				return;
			}
			
			// loaded a new scene
			if (gGuiSettings.combos.textureBlob == 0)
				reloadTexture = true;
			
			int textureBlobIndex = gGuiSettings.combos.textureBlobIndex;
			struct TextureBlob *textureBlob = &gScene->textureBlobs[textureBlobIndex];
			gGuiSettings.combos.textureBlob = textureBlob;
			
			if (!imageData)
			{
				// n64 tmem = 4kib, *4 for 32-bit color conversion
				// and *2 b/c 4bit textures expand to *2*4x the bytes
				//imageData = (uint8_t*)malloc(4096 * 4 * 2);
				imageData = (uint8_t*)calloc(4, 512 * 512); // for prerenders
				
				/* test
				imageWidth = imageHeight = 64;
				for (int i = 0; i < imageWidth * imageHeight; ++i)
				{
					uint8_t *color = imageData + i * 4;
					
					color[0] = i * 1;
					color[1] = i * 2;
					color[2] = i * 3;
					color[3] = 0xff;
				}
				*/
			}
			
			if (reloadTexture)
			{
				reloadTexture = false;
				
				if (textureBlob)
				{
					if (imageTexture)
					{
						glDeleteTextures(1, &imageTexture);
						imageTexture = 0;
					}
					
					struct DataBlob *blob = textureBlob->data;
					struct DataBlob *palBlob = blob->data.texture.pal;
					imageWidth = blob->data.texture.w;
					imageHeight = blob->data.texture.h;
					imageFmt = blob->data.texture.fmt;
					imageSiz = blob->data.texture.siz;
					segAddr = blob->originalSegmentAddress;
					palAddr = palBlob ? palBlob->originalSegmentAddress : 0;
					int sizeBytesClamped = blob->data.texture.sizeBytesClamped;
					
					isBadTexture = false;
					if (sizeBytesClamped > 4096
						|| (((uint8_t*)blob->refData) + sizeBytesClamped) > blob->refDataFileEnd
						// bounds checking for palette, if applicable:
						|| (palBlob && (((uint8_t*)palBlob->refData) + palBlob->sizeBytes) > palBlob->refDataFileEnd)
					)
					{
						LogDebug("warning: width height %d x %d", imageWidth, imageHeight);
						LogDebug("refData    = %p", blob->refData);
						LogDebug("refDataEnd = %p", blob->refDataFileEnd);
						LogDebug("sizeBytes  = %x", blob->sizeBytes);
						isBadTexture = true;
						goto L_textureError;
					}
					
					if (blob->data.texture.isJfif)
					{
						int x, y, c;
						void *test = stbi_load_from_memory(
							(const stbi_uc*)blob->refData
							, blob->sizeBytes
							, &x, &y, &c, 4
						);
						//LogDebug("test = %p %d x %d x %d", test, x, y, c);
						memcpy(imageData, test, x * y * 4);
						stbi_image_free(test);
					}
					else
					{
						n64texconv_to_rgba8888(
							imageData
							, (unsigned char*)blob->refData // TODO const correctness
							, (unsigned char*)(palBlob ? palBlob->refData : 0)
							, (n64texconv_fmt)imageFmt
							, (n64texconv_bpp)imageSiz
							, imageWidth
							, imageHeight
							, blob->data.texture.lineSize
						);
					}
					
					// Create a OpenGL texture identifier
					glGenTextures(1, &imageTexture);
					glBindTexture(GL_TEXTURE_2D, imageTexture);
					
					// Setup filtering parameters for display
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same
					
					// Upload pixels into texture
				#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
					glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
				#endif
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
				}
			}
		
		L_textureError:
			ImGui::Text("%d/%d: tex %08x%s"
				, textureBlobIndex
				, sb_count(gScene->textureBlobs) - 1
				, segAddr
				, palAddr ? "," : ""
			);
			if (palAddr) {
				ImGui::SameLine();
				ImGui::Text("pal %08x", palAddr);
			}
			const char *fmt[] = { "rgba", "yuv", "  ci", "  ia", "   i" };
			const char *bpp[] = { "4 ", "8 ", "16", "32" };
			if (textureBlob->data->data.texture.isJfif)
				ImGui::Text("jfif");
			else
				ImGui::Text("%s-%s", fmt[imageFmt], bpp[imageSiz]);
			ImGui::SameLine();
			ImGui::Text("%3d x %3d", imageWidth, imageHeight);
			ImGui::SameLine();
			// buttons
			if (ImGui::Button("<-##Textures"))
			{
				textureBlobIndex = WRAP(
					textureBlobIndex - 1
					, 0
					, sb_count(gScene->textureBlobs) - 1
				);
			}
			ImGui::SameLine();
			if (ImGui::Button("->##Textures"))
			{
				textureBlobIndex = WRAP(
					textureBlobIndex + 1
					, 0
					, sb_count(gScene->textureBlobs) - 1
				);
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(48);
			ImGui::Combo(
				"##Textures##Scale"
				, &scale
				, [](void* data, int n) {
					static char test[64];
					sprintf(test, "%d", n + 1);
					return (const char*)test;
				}
				, 0
				, scaleMax
			);
			IMGUI_COMBO_HOVER(scale, scaleMax);
			
			ImGui::Text(textureBlob->file->shortname);
			
			if (isBadTexture)
				ImGui::Text("Bad texture");
			else if (imageTexture)
			{
				ImGui::Image((void*)(intptr_t)imageTexture
					, ImVec2(imageWidth * (scale + 1)
					, imageHeight * (scale + 1))
				);
				if (ImGui::Button("Export to PNG##Textures"))
				{
					const char *fn;
					char name[128];
					const char *fmt[] = { "rgba", "yuv", "ci", "ia", "i" };
					const char *bpp[] = { "4", "8", "16", "32" };
					struct DataBlob *blob = textureBlob->data;
					struct DataBlob *palBlob = blob->data.texture.pal;
					
					snprintf(name, sizeof(name), "%08x.%s%s.png", segAddr, fmt[imageFmt], bpp[imageSiz]);
					
					if (textureBlob->data->data.texture.isJfif)
						snprintf(name, sizeof(name), "%08x.jfif.png", segAddr);
					
					fn = noc_file_dialog_open(NOC_FILE_DIALOG_SAVE, "png\0*.png\0", NULL, name);
					
					if (fn)
						stbi_write_png(fn, imageWidth, imageHeight, 4, imageData, 0);
				}
			}
			
			if (textureBlobIndex != gGuiSettings.combos.textureBlobIndex)
			{
				gGuiSettings.combos.textureBlobIndex = textureBlobIndex;
				reloadTexture = true;
			}
		}
	},
	new LinkedStringFunc{
		"Project"
		, []() {
			Project *project = gGuiSettings.project;
			
			ImGui::Text("contains %d scenes", sb_count(project->scenes));
			static int current = -1;
			if (ImGui::BeginListBox("##ProjectSceneList", ImVec2(-FLT_MIN, 5 * ImGui::GetTextLineHeightWithSpacing())))
			{
				sb_foreach(project->scenes, {
					char tmp[256];
					const char *name = each->name;
					if (!name) {
						name = tmp;
						snprintf(tmp, sizeof(tmp), "%08x - %08x", each->startAddress, each->endAddress);
					}
					//ImGui::Text(name);
					const bool is_selected = (current == eachIndex);
					if (ImGui::Selectable(name, is_selected)) {
						current = eachIndex;
						LogDebug("load scene %d", current);
						WindowLoadSceneFromRom(project->file, each->startAddress, each->endAddress);
					}
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				})
				ImGui::EndListBox();
			}
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
		
		// when an instance is clicked, automatically switch
		// to the tab containing information about it
		ON_CHANGE(gGui->selectedInstance)
		{
			if (gGui->selectedInstance)
			{
				const char *tabName = 0;
				
				switch (gGui->selectedInstance->tab)
				{
					case INSTANCE_TAB_ACTOR: tabName = TABNAME_ACTORS; break;
					case INSTANCE_TAB_DOOR: tabName = TABNAME_DOORS; break;
					case INSTANCE_TAB_SPAWN: tabName = TABNAME_SPAWNS; break;
					default: Die("unknown tab type");
				}
				
				if (tabName)
				{
					for (int i = 0; i < sizeof(gSidebarTabs) / sizeof(*gSidebarTabs); ++i)
					{
						if (!strcmp(gSidebarTabs[i]->name, tabName))
						{
							which = i;
							break;
						}
					}
				}
			}
		}
		
		int numTabs = sizeof(gSidebarTabs) / sizeof(*gSidebarTabs);
		MultiLineTabBarGeneric(
			"SidebarTabs"
			, reinterpret_cast<const void**>(gSidebarTabs)
			, numTabs - (!gGuiSettings.project)
			, 6 - (!!gGuiSettings.project)
			, &which
			, [](const void **arr, const int index) -> const char* {
				const LinkedStringFunc **ready = reinterpret_cast<const LinkedStringFunc**>(arr);
				return ready[index]->name;
			}
		);
		//ImGui::TextWrapped(gSidebarTabs[which]->name); // test
		
		// draw the selected sidebar
		if (gScene || which == numTabs - 1)
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
	
	if (gGui->rightClickedInViewport)
		ImGui::OpenPopup("##RightClickWorldMenu");
	if (ImGui::BeginPopup("##RightClickWorldMenu"))
	{
		RenderGroup renderGroup = gGui->rightClickedRenderGroup;
		uint32_t group = renderGroup & RENDERGROUP_MASK_GROUP;
		uint32_t id = renderGroup & RENDERGROUP_MASK_ID;
		
		// right-clicked room geometry
		if (group == RENDERGROUP_ROOM)
		{
			// current room
			if (id == gGui->selectedRoomIndex)
			{
				if (ImGui::Selectable("Add Actor Here"))
				{
					QUEUE_POPUP(AddNewInstanceSearch);
					gAddNewInstanceSearchTab = INSTANCE_TAB_ACTOR;
				}
				if (ImGui::Selectable("Add Spawn Point Here"))
				{
					QUEUE_POPUP(AddNewInstanceSearch);
					gAddNewInstanceSearchTab = INSTANCE_TAB_SPAWN;
				}
				if (ImGui::Selectable("Add Doorway Here"))
				{
					QUEUE_POPUP(AddNewInstanceSearch);
					gAddNewInstanceSearchTab = INSTANCE_TAB_DOOR;
				}
				if (ImGui::MenuItem("Paste Here", "Ctrl+V"))
				{
					WindowTryInstancePaste(true, false);
				}
			}
			// not current room
			else
			{
				if (ImGui::Selectable("Select this room"))
				{
					gGui->selectedRoomIndex = id;
				}
			}
		}
		// right-clicked an instance
		else if (group == RENDERGROUP_INST)
		{
			if (ImGui::MenuItem("Delete", "Del"))
			{
				WindowTryInstanceDelete(true);
			}
			if (ImGui::MenuItem("Cut", "Ctrl+X"))
			{
				WindowTryInstanceCut(true);
			}
			if (ImGui::MenuItem("Copy", "Ctrl+C"))
			{
				WindowTryInstanceCopy(true);
			}
			if (ImGui::MenuItem("Duplicate", "Shift+D"))
			{
				WindowTryInstanceDuplicate();
			}
		}
		
		ImGui::EndPopup();
	}
	
	// misc popups
	DEQUEUE_POPUP(AddNewInstanceSearch);
	if (ImGui::BeginPopup(STRINGIFY(AddNewInstanceSearch)))
	{
		auto &type = InstanceTypeSearch();
		
		if (!type.isEmpty)
		{
			LogDebug("add instance of type '%s'", type.name);
			
			switch (gAddNewInstanceSearchTab)
			{
				case INSTANCE_TAB_ACTOR: gGui->instanceList = gGui->actorList; break;
				case INSTANCE_TAB_SPAWN: gGui->instanceList = gGui->spawnList; break;
				case INSTANCE_TAB_DOOR: gGui->instanceList = gGui->doorList; break;
				default: Die("unknown tab type"); break;
			}
			
			struct Instance newInst = {
				.id = type.index,
				.pos = gGui->newSpawnPos,
				.tab = gAddNewInstanceSearchTab,
				.prev = INSTANCE_PREV_INIT,
			};
			newInst.mm.halfDayBits = 0xffff; // default = appear on all days (oot and mm)
			
			if (gGui->instanceList)
				gGui->selectedInstance = &sb_push(*(gGui->instanceList), newInst);
			else
				LogDebug("instanceList == 0, can't add");
		}
		
		ImGui::EndPopup();
	}
	DEQUEUE_POPUP(AddNewObjectSearch);
	if (ImGui::BeginPopup(STRINGIFY(AddNewObjectSearch)))
	{
		auto &type = ObjectTypeSearch();
		
		if (!type.isEmpty)
		{
			LogDebug("add object of type '%s'", type.name);
			
			struct ObjectEntry newObj = {
				.name = type.name,
				.id = type.index,
				.type = OBJECT_ENTRY_TYPE_EXPLICIT,
			};
			
			if (gGui->objectList)
				gGui->selectedObject = &sb_push(*(gGui->objectList), newObj);
		}
		
		ImGui::EndPopup();
	}
}

static void DrawMenuBar(void)
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::BeginMenu("New"))
			{
				if (ImGui::MenuItem("Scene from OBJEX", "Ctrl+N"))
				{
					WindowNewSceneFromObjex(0, 0, true);
				}
				ImGui::EndMenu();
			}
			if (ImGui::MenuItem("Open", "Ctrl+O"))
			{
				LogDebug("wow!");
				
				Scene *newScene = WindowOpenFile();
				
				if (newScene && newScene != gScene)
				{
					gGuiSettings.Reset();
					gScene = newScene;
				}
			}
			if (ImGui::MenuItem("Save", "Ctrl+S", false, gScene && gScene->file->ownsData))
			{
				WindowSaveScene();
			}
			if (ImGui::MenuItem("Save as...", "Ctrl+Shift+S", false, !!gScene))
			{
				WindowSaveSceneAs();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Exit", "Ctrl+Q"))
			{
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Profile"))
		{
			if (ImGui::MenuItem("Load zzrtl/z64rom project"))
			{
				const char *fn;
				
				fn = noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, "z64 romhack projects\0*.rtl;*.zzrpl;*.toml\0", NULL, NULL);
				
				if (fn)
				{
					gGuiSettings.project = ProjectNewFromFilename(fn);
					
					// game (oot or mm) comes from project
					LogDebug("project->game = '%s'", gGuiSettings.project->game);
					gGuiSettings.projectIsReady = false;
					GuiLoadBaseDatabases(gGuiSettings.project->game);
					gGuiSettings.projectIsReady = true;
					
					TomlInjectDataFromProject(
						gGuiSettings.project
						, &gGuiSettings.actorDatabase
						, &gGuiSettings.objectDatabase
					);
				}
			}
			else if (ImGui::MenuItem("Load scenes from rom"))
			{
				const char *fn;
				
				fn = noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, "Zelda 64 rom files\0*.z64\0", NULL, NULL);
				
				if (fn)
				{
					gGuiSettings.project = ProjectNewFromFilename(fn);
					
					// game (oot or mm) comes from project
					LogDebug("project->game = '%s'", gGuiSettings.project->game);
					gGuiSettings.projectIsReady = false;
					GuiLoadBaseDatabases(gGuiSettings.project->game);
					gGuiSettings.projectIsReady = true;
					
					TomlInjectDataFromProject(
						gGuiSettings.project
						, &gGuiSettings.actorDatabase
						, &gGuiSettings.objectDatabase
					);
				}
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
		ImGui::Text("%.02f fps", ImGui::GetIO().Framerate);
		
		// menu bar items, aligned right-to-left
		if (gScene)
		{
			int pad = 10;
			int right = ImGui::GetWindowWidth();
			int width;
			
			#define WITH_WIDTH(X) \
				width = X; \
				ImGui::SameLine((right -= (width + pad))); \
				ImGui::SetNextItemWidth(width);
			
			// room select
			WITH_WIDTH(80)
			ImGui::Combo(
				"##MenuBar##RoomIndex"
				, &gGui->selectedRoomIndex
				, [](void* data, int n) {
					static char test[256];
					sprintf(test, "Room %d", n);
					return (const char*)test;
				}
				, 0
				, sb_count(gScene->rooms)
			);
			IMGUI_COMBO_HOVER(gGui->selectedRoomIndex, sb_count(gScene->rooms));
			
			// header select
			int numHeaders = sb_count(gScene->headers);
			WITH_WIDTH(128)
			ImGui::Combo(
				"##MenuBar##HeaderIndex"
				, &gGui->selectedHeaderIndex
				, [](void* data, int n) {
					static char test[256];
					sprintf(test, "Header %d", n);
					if (n == sb_count(gScene->headers))
						sprintf(test, "Add new header##AddNewHeader");
					return (const char*)test;
				}
				, 0
				, numHeaders + 1 // last one = add new header
			);
			IMGUI_COMBO_HOVER(gGui->selectedHeaderIndex, numHeaders);
			if (gGui->selectedHeaderIndex == numHeaders)
				SceneAddHeader(gScene, 0);
			
			// day select
			gGui->halfDayBits = 0xffff; // using for oot and mm (mm overrides it)
			if (gGui->isMM)
			{
				const int days = 11;
				WITH_WIDTH(80)
				ImGui::Combo(
					"##MenuBar##DayIndex"
					, &gGui->selectedDayIndex
					, [](void* data, int n) {
						static char test[256];
						if (n == 0)
							sprintf(test, "All");
						else if (n & 1)
							sprintf(test, "Day %d", (n + 1) / 2);
						else
							sprintf(test, "Night %d", (n + 1) / 2);
						return (const char*)test;
					}
					, 0
					, days
				);
				IMGUI_COMBO_HOVER(gGui->selectedDayIndex, days);
				
				// calculate new halfDayBits
				{
					int i = gGui->selectedDayIndex;
					
					if (i == 0)
						gGui->halfDayBits = 0xffff;
					else {
						int shift = 9 - (i - 1);
						gGui->halfDayBits = 1 << shift;
					}
				}
			}
			
			// if any of those changed, update accordingly
			bool reset = false;
			ON_CHANGE(gGui->selectedDayIndex) reset = true;
			ON_CHANGE(gGui->selectedRoomIndex) reset = true;
			ON_CHANGE(gGui->selectedHeaderIndex) reset = true;
			if (reset) {
				gGui->selectedInstance = 0;
			}
			
			#undef WITH_WIDTH
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
	
	// turn off imgui.ini
	ImGuiIO &io = ImGui::GetIO();
	io.IniFilename = NULL;
	io.LogFilename = NULL;
	
	//JsonTest();
	//TomlTest();
	//gGuiSettings.actorDatabase = TomlLoadActorDatabase("toml/game/oot/actors.toml");
	//gGuiSettings.objectDatabase = TomlLoadObjectDatabase("toml/game/oot/objects.toml");
	//TestAllActorRenderCodeGen();
	
	// test modals
	return;
	gGuiSettings.PushModal("hello world");
	gGuiSettings.PushModal("hello world-1");
	gGuiSettings.PushModal("hello world-2");
}

extern "C" void GuiCleanup(void)
{
	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

extern "C" void GuiSetInterop(struct GuiInterop *interop)
{
	gGui = interop;
}

extern "C" void GuiDraw(GLFWwindow *window, struct Scene *scene, struct GuiInterop *interop)
{
	// reset settings on extern scene load (handles Ctrl+O and dropped files)
	if (scene != gScene)
		gGuiSettings.Reset();
	
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
	
	// handling this here for now
	if (scene)
	{
		RoomHeader *roomHeader = &gScene->rooms[gGui->selectedRoomIndex].headers[gGui->selectedHeaderIndex];
		SceneHeader *sceneHeader = &gScene->headers[gGui->selectedHeaderIndex];
		
		gGui->sceneHeader = sceneHeader;
		gGui->doorList = &(sceneHeader->doorways);
		gGui->spawnList = &(sceneHeader->spawns);
		gGui->actorList = &(roomHeader->instances);
		gGui->objectList = &(roomHeader->objects);
		
		// now that door/spawn/actor/object lists are non-zero, watch for changes
		bool refreshObjects = false;
		ON_CHANGE(sb_count(*gGui->doorList)) refreshObjects = true;
		ON_CHANGE(sb_count(*gGui->actorList)) refreshObjects = true;
		ON_CHANGE(sb_count(*gGui->objectList)) refreshObjects = true;
		ON_CHANGE(gGui->sceneHeader->specialFiles) refreshObjects = true;
		if(gGui->sceneHeader->specialFiles) { ON_CHANGE(gGui->sceneHeader->specialFiles->subkeepObjectId) refreshObjects = true; }
		if (refreshObjects)
			RefreshObjectStats();
	}
	
	if (gGuiSettings.showSidebar)
		DrawSidebar();
	
	gGuiSettings.DrawModals();
	
	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	if (gGuiSettings.showImGuiDemoWindow)
		ImGui::ShowDemoWindow(&gGuiSettings.showImGuiDemoWindow);
	
	// error popup
	ON_CHANGE(gGuiSettings.errorMessagePopupBody)
	{
		if (gGuiSettings.errorMessagePopupBody)
			ImGui::OpenPopup(GUI_ERROR_POPUP_ID);
	}
	if (gGuiSettings.errorMessagePopupBody)
	{
		// center the popup
		ImGui::SetNextWindowPos(
			ImVec2(
				ImGui::GetIO().DisplaySize.x * 0.5f,
				ImGui::GetIO().DisplaySize.y * 0.5f
			),
			ImGuiCond_Always,
			ImVec2(0.5f,0.5f)
		);
		if (ImGui::BeginPopupModal(GUI_ERROR_POPUP_ID, NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::TextWrapped(gGuiSettings.errorMessagePopupBody);
			ImGui::Separator();
			
			ImGui::SetItemDefaultFocus();
			if (ImGui::Button("OK", ImVec2(120, 0))) {
				ImGui::CloseCurrentPopup();
				gGuiSettings.errorMessagePopupBody = 0;
			}
		}
	}
	
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

extern "C" int GuiHasFocusMouse(void)
{
	return ImGui::GetIO().WantCaptureMouse;
}

extern "C" int GuiHasFocusKeyboard(void)
{
	return ImGui::GetIO().WantCaptureKeyboard;
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

extern "C" void GuiPushModal(const char *message)
{
	gGuiSettings.PushModal(message);
}

// gets vm, or source for compilation, if no vm exists yet
// intended use: C gets src, creates VM, sends back VM
extern "C" struct ActorRenderCode *GuiGetActorRenderCode(uint16_t id)
{
	if (!gGuiSettings.project)
		return 0;
	
	auto &actor = gGuiSettings.actorDatabase.GetEntry(id);
	
	if (actor.isEmpty)
		return 0;
	
	struct ActorRenderCode *rc = &actor.rendercode;
	
	if (rc->type == ACTOR_RENDER_CODE_TYPE_UNINITIALIZED)
	{
		for (auto objectId : actor.objects)
		{
			auto &object = gGuiSettings.objectDatabase.GetEntry(objectId);
			
			object.TryLoadSyms();
			object.PrintAllSyms();
		}
		
		const char *tmp = actor.RenderCodeGen(GetObjectSymbolAddresses);
		
		if (tmp)
		{
			rc->type = ACTOR_RENDER_CODE_TYPE_SOURCE;
			rc->src = tmp;
		}
		else
		{
			return 0;
		}
	}
	
	return rc;
}

// each property gets hooks created for it
extern "C" void GuiCreateActorRenderCodeHandles(uint16_t id)
{
	auto &actor = gGuiSettings.actorDatabase.GetEntry(id);
	
	if (actor.isEmpty)
		return;
	
	if (actor.rendercode.type == ACTOR_RENDER_CODE_TYPE_VM)
		actor.CreateRenderCodeHandles();
}

// populates properties from instance to rendercode VM
extern "C" void GuiApplyActorRenderCodeProperties(struct Instance *inst)
{
	auto &actor = gGuiSettings.actorDatabase.GetEntry(inst->id);
	
	if (actor.isEmpty)
		return;
	
	if (actor.rendercode.type == ACTOR_RENDER_CODE_TYPE_VM)
		actor.ApplyRenderCodeProperties(inst->params, inst->xrot, inst->yrot, inst->zrot);
}

extern "C" int GuiGetActorObjectIdFromSlot(uint16_t actorId, int slot)
{
	auto &actor = gGuiSettings.actorDatabase.GetEntry(actorId);
	
	if (actor.isEmpty)
		return -1;
	
	if (slot >= actor.objects.size())
		return -1;
	
	return actor.objects[slot];
}

extern "C" struct Object *GuiGetObjectDataFromId(int objectId)
{
	if (objectId <= 0)
		return 0;
	
	auto &object = gGuiSettings.objectDatabase.GetEntry(objectId);
	
	if (object.isEmpty)
		return 0;
	
	if (object.zobjData)
		return object.zobjData;
	
	object.TryLoadData();
	return 0;
}

extern "C" void GuiLoadBaseDatabases(const char *gameId)
{
	char tmp[256];
	
	// unsafe
	if (!gGui)
		return;
	
	// don't override databases if a project is already loaded
	if (gGuiSettings.projectIsReady)
		return;
	
	if (gGui)
		gGui->isMM = !strcmp(gameId, "mm");
	
	snprintf(tmp, sizeof(tmp), "toml/game/%s/actors.toml", gameId);
	gGuiSettings.actorDatabase = TomlLoadActorDatabase(tmp);
	snprintf(tmp, sizeof(tmp), "toml/game/%s/objects.toml", gameId);
	gGuiSettings.objectDatabase = TomlLoadObjectDatabase(tmp);
}

// basic popup with 'ok' button
extern "C" void GuiErrorPopup(const char *message)
{
	LogDebug("set error popup = '%s'", message);
	gGuiSettings.errorMessagePopupBody = message;
}

// for testing all the things
extern "C" void GuiTest(Project *project)
{
	gGuiSettings.actorDatabase = TomlLoadActorDatabase("toml/game/oot/actors.toml");
	gGuiSettings.objectDatabase = TomlLoadObjectDatabase("toml/game/oot/objects.toml");
	
	// testing rendercode
	bool RenderCodeGo(struct Instance *inst);
	Instance instance = {};
	instance.id = 0x0002;
	for (int i = 0; i < 10; ++i)
		RenderCodeGo(&instance);
	return;
	
	TomlInjectDataFromProject(project
		, &gGuiSettings.actorDatabase
		, &gGuiSettings.objectDatabase
	);
	
	// test
	{
		auto &obj = gGuiSettings.objectDatabase.GetEntry(50);
		obj.TryLoadSyms();
		obj.PrintAllSyms();
	}
	
	// codegen test
	TestAllActorRenderCodeGen();
}

