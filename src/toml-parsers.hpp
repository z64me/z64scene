//
// toml-parsers.hpp <z64scene>
//
// toml parsers live here, since
// it's relatively slow to compile
//

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include <iostream>
#include <vector>
#include <string>
#include <map>

extern "C" {
#include "project.h"
#include "file.h"
#include "rendercode.h"
#include "object.h"
#include "logging.h"
}

#define SPAN_UPPERCASE   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define SPAN_LOWERCASE   "abcdefghijklmnopqrstuvwxyz"
#define SPAN_DIGITS      "0123456789"
#define SPAN_UNDERSCORE  "_"
#define SPAN_VARNAME \
	SPAN_UPPERCASE \
	SPAN_LOWERCASE \
	SPAN_DIGITS \
	SPAN_UNDERSCORE

#define STRTOK_LOOP(STRING, DELIM) \
	for (char *next, *each = strtok(STRING, DELIM) \
		; each && (next = strtok(0, DELIM)) \
		; each = next \
	)

#define STRCATF(DST, FMT, ...) { sprintf(DST, FMT, ##__VA_ARGS__); DST += strlen(DST); }

struct ActorDatabase
{
	struct Entry
	{
		struct Property
		{
			struct Combo
			{
				const char   *label;
				uint16_t      value;
			};
			
			const char          *target;
			const char          *name;
			uint16_t             mask;
			std::vector<Combo>   combos;
			WrenHandle *slotHandle = 0;
			WrenHandle *callHandle = 0;
			
			void AddCombo(uint16_t value, const char *label)
			{
				Combo combo = { label, value };
				
				combos.emplace_back(combo);
			}
			
			uint16_t *GetWhich(uint16_t *params, uint16_t *xrot, uint16_t *yrot, uint16_t *zrot)
			{
				uint16_t *which =
					!strcmp(target, "Var") ? params
					: !strcmp(target, "XRot") ? xrot
					: !strcmp(target, "YRot") ? yrot
					: !strcmp(target, "ZRot") ? zrot
					: 0;
				
				return which;
			}
			
			uint16_t Extract(uint16_t params, uint16_t xrot, uint16_t yrot, uint16_t zrot)
			{
				uint16_t *which = GetWhich(&params, &xrot, &yrot, &zrot);
				
				if (!which || !mask)
					return 0;
				
				uint16_t tmp = mask;
				uint16_t result = (*which) & mask;
				while (!(tmp & 1))
					tmp >>= 1, result >>= 1;
				
				return result;
			}
			
			void Inject(uint16_t v, uint16_t *params, uint16_t *xrot, uint16_t *yrot, uint16_t *zrot)
			{
				uint16_t *which = GetWhich(params, xrot, yrot, zrot);
				
				if (!which || !mask)
					return;
				
				uint16_t tmp = mask;
				while (!(tmp & 1))
					tmp >>= 1, v <<= 1;
				*which &= ~mask;
				*which |= v;
			}
			
			const char *QuickSanitizedName(const char *suffix = 0)
			{
				static char work[256];
				const char *src = name;
				for (char *dst = work; ; ++dst, ++src)
				{
					*dst = *src;
					if (!*dst)
						break;
					// whitespace is excluded
					// underscores are allowed
					// break on any other non-alphanumeric chars
					if (!isalnum(*dst)) {
						if (strchr(" \t", *dst))
							--dst;
						else if (*dst != '_') {
							*dst = 0;
							break;
						}
					}
				}
				if (suffix)
					strcat(work, suffix);
				return work;
			}
		};
		
		char                   *name;
		uint16_t                index;
		std::vector<uint16_t>   objects;
		std::vector<Property>   properties;
		bool isEmpty = false;
		int categoryInt = -1;
		char *rendercodeToml = 0;
		uint32_t rendercodeLineNumberInToml = 0;
		uint32_t rendercodeLineNumberOffset = 0;
		ActorRenderCode rendercode = {};
		// compilation errors should report:
		// (lineError - rendercodeLineNumberOffset) + rendercodeLineNumberInToml
		
		void AddProperty(Property property)
		{
			properties.emplace_back(property);
		}
		
		const char *RenderCodeGen(
			std::map<std::string, uint32_t> GetObjectSymbolAddresses(uint16_t objId)
		)
		{
			if (!rendercodeToml)
				return 0;
			
			static char *work = 0;
			
			if (!work)
				work = (char*)malloc(4096);
			
			char *buf = work;
			*buf = '\0';
			STRCATF(buf, "%s", R"(
				class Inst {
					foreign static Xpos
					foreign static Ypos
					foreign static Zpos
					foreign static Xrot
					foreign static Yrot
					foreign static Zrot
					foreign static PositionChanged
					foreign static PropertyChanged
					foreign static Uuid
				}
				class Draw {
					foreign static SetScale(xscale, yscale, zscale)
					foreign static SetScale(scale)
					foreign static UseObjectSlot(slot)
					foreign static Mesh(address)
					foreign static Skeleton(addressOrIndex)
					foreign static SetLocalPosition(xpos, ypos, zpos)
					foreign static SetGlobalRotation(xrot, yrot, zrot)
					foreign static SetPrimColor(r, g, b)
					foreign static SetPrimColor(r, g, b, a)
				}
				class Math {
					foreign static SinS(s16angle)
					foreign static CosS(s16angle)
				}
			)");
			
			// properties
			STRCATF(buf, "%s", R"(
				class PropsClass {
			)");
			for (auto &option : properties) {
				const char *name = option.QuickSanitizedName();
				STRCATF(buf, "%s { _%s }\n", name, name);
				STRCATF(buf, "%s=(v) { _%s = v }\n", name, name);
			}
			STRCATF(buf, "%s", R"(
				construct new() { }
				}
				var Props = PropsClass.new()
			)");
			
			// constants
			STRCATF(buf, "%s", R"(
				class SymsClass {
			)");
			for (int i = 0; i < objects.size(); ++i)
			{
				auto symbolAddresses = GetObjectSymbolAddresses(objects[i]);
				
				for (const auto &sym : symbolAddresses)
					STRCATF(buf, "%s { 0x%08x }\n", sym.first.c_str(), sym.second);
			}
			STRCATF(buf, "%s", R"(
					construct new() { }
				}
				var Syms = SymsClass.new()
			)");
			
			// codegen for udata class
			const char udataTok[] = "Udata.";
			const int udataTokLen = sizeof(udataTok) - 1;
			const char *udataMatch = strstr(rendercodeToml, udataTok);
			bool usesUdata = false;
			if (udataMatch)
			{
				usesUdata = true;
				std::map<std::string, bool> udataMembers = {};
				char tmp[256];
				
				do {
					udataMatch += udataTokLen;
					int len = strspn(udataMatch, SPAN_VARNAME);
					if (len > 0 && len < sizeof(tmp) - 1) {
						memcpy(tmp, udataMatch, len);
						tmp[len] = '\0';
						udataMembers[tmp] = true;
					}
					udataMatch = strstr(udataMatch, udataTok);
				} while (udataMatch);
				
				STRCATF(buf, R"(
					class UdataClass {
				)");
				for (auto &member : udataMembers) {
					const char *name = member.first.c_str();
					STRCATF(buf, "%s { _%s }\n", name, name);
					STRCATF(buf, "%s=(v) { _%s = v }\n", name, name);
				}
				STRCATF(buf, R"(
					construct new() { }
					}
					var UdataMap = {}
				)");
			}
			
			// rendercode prefix
			STRCATF(buf, R"(
				class hooks {
					static draw() {
						Draw.UseObjectSlot(0)
						Draw.SetLocalPosition(0, 0, 0)
						Draw.SetScale(0.1)
			)");
			
			if (usesUdata)
				STRCATF(buf, R"(
					var Udata = UdataMap[Inst.Uuid.toString]
					if (Udata is Null) {
						Udata = UdataClass.new()
						UdataMap[Inst.Uuid.toString] = Udata
						//System.print("allocate Udata for %(Inst.Uuid) ")
					}
				)");
			
			// render code offset
			rendercodeLineNumberOffset = 2;
			for (char *tmp = work; *tmp; ++tmp)
				if (*tmp == '\n')
					rendercodeLineNumberOffset += 1;
			
			// rendercode
			STRCATF(buf, " %s \n } } ", rendercodeToml);
			
			// add to error line number, to report correct line in toml
			rendercode.lineErrorOffset = 0;
			rendercode.lineErrorOffset -= rendercodeLineNumberOffset;
			rendercode.lineErrorOffset += rendercodeLineNumberInToml;
			
			return work;
		}
		
		void CreateRenderCodeHandles(void)
		{
			if (rendercode.type != ACTOR_RENDER_CODE_TYPE_VM)
				return;
			
			WrenVM *vm = rendercode.vm;
			const char* module = "main";
			for (auto &prop : properties)
			{
				wrenEnsureSlots(vm, 2);
				wrenGetVariable(vm, module, "Props", 0);
				
				prop.slotHandle = wrenGetSlotHandle(vm, 0);
				prop.callHandle = wrenMakeCallHandle(vm, prop.QuickSanitizedName("=(_)"));
				
				sb_push(rendercode.vmHandles, prop.slotHandle);
				sb_push(rendercode.vmHandles, prop.callHandle);
			}
		}
		
		void ApplyRenderCodeProperties(uint16_t params, uint16_t xrot, uint16_t yrot, uint16_t zrot)
		{
			if (rendercode.type != ACTOR_RENDER_CODE_TYPE_VM)
				return;
			
			WrenVM *vm = rendercode.vm;
			for (auto &prop : properties)
			{
				wrenEnsureSlots(vm, 2);
				wrenSetSlotHandle(vm, 0, prop.slotHandle);
				wrenSetSlotDouble(vm, 1, prop.Extract(params, xrot, yrot, zrot));
				wrenCall(vm, prop.callHandle);
			}
		}
	};
	
	std::vector<Entry>   entries;
	
	void AddEntry(Entry entry)
	{
		// make sure it will fit
		if (entry.index >= entries.size())
			entries.resize(entry.index + 1);
		
		// TODO if already occupied, do cleanup
		
		entries[entry.index] = entry;
	}
	
	Entry &GetEntry(uint16_t index)
	{
		if (index >= entries.size())
		{
			static Entry empty;
			empty.isEmpty = true;
			
			return empty;
		}
		
		return entries[index];
	}
	
	void RemoveEntry(uint16_t index)
	{
		auto &tmp = GetEntry(index);
		
		if (tmp.name)
			free(tmp.name);
		
		if (tmp.rendercodeToml)
			free(tmp.rendercodeToml);
		
		tmp.name = 0;
		tmp.rendercodeToml = 0;
		tmp.rendercode = (ActorRenderCode){};
		tmp.isEmpty = true;
		tmp.objects = std::vector<uint16_t>();
		tmp.properties = std::vector<Entry::Property>();
	}
	
	const char *GetActorName(uint16_t index)
	{
		auto &entry = GetEntry(index);
		
		if (!entry.name)
			return "unknown";
		
		return entry.name;
	}
};

struct ObjectDatabase
{
	struct Entry
	{
		char *name;
		char *zobjPath;
		char *symsPath;
		Object *zobjData = 0;
		uint16_t index;
		std::map<std::string, uint32_t> symbolAddresses;
		bool isEmpty = false;
		
		void TryLoadData(void)
		{
			if (!zobjPath)
				return;
			
			if (FileExists(zobjPath))
				zobjData = ObjectFromFilename(zobjPath);
			
			// consider this consumed
			free(zobjPath);
			zobjPath = 0;
		}
		
		void TryLoadSyms(void)
		{
			if (!symsPath)
				return;
			
			if (FileExists(symsPath))
			{
				File *tmp = FileFromFilename(symsPath);
				STRTOK_LOOP((char*)tmp->data, "\r\n\t =;") {
					uint32_t v;
					if (*next == '\0')
						continue;
					if (sscanf(next, "%x", &v) != 1)
						continue;
					symbolAddresses[each] = v;
					*next = '\0'; // mark consumed
				}
				FileFree(tmp);
			}
			else
				LogDebug("'%s' doesn't exist", symsPath);
			
			// consider these consumed
			free(symsPath);
			symsPath = 0;
		}
		
		void PrintAllSyms(void)
		{
			// print contents
			for (const auto &sym : symbolAddresses)
				LogDebug("%s = 0x%08x;", sym.first.c_str(), sym.second);
				//std::cout << sym.first << " = " << std::hex << sym.second << ";\n";
		}
	};
	
	std::vector<Entry> entries;
	
	void AddEntry(Entry entry)
	{
		// make sure it will fit
		if (entry.index >= entries.size())
			entries.resize(entry.index + 1);
		
		// TODO if already occupied, do cleanup
		
		entries[entry.index] = entry;
	}
	
	Entry &GetEntry(uint16_t index)
	{
		if (index >= entries.size())
		{
			static Entry empty;
			empty.isEmpty = true;
			
			return empty;
		}
		
		return entries[index];
	}
	
	void RemoveEntry(uint16_t index)
	{
		auto &tmp = GetEntry(index);
		
		if (tmp.name)
			free(tmp.name);
		if (tmp.zobjPath)
			free(tmp.zobjPath);
		if (tmp.symsPath)
			free(tmp.symsPath);
		
		tmp.name = 0;
		tmp.zobjPath = 0;
		tmp.symsPath = 0;
		tmp.isEmpty = true;
		tmp.symbolAddresses = std::map<std::string, uint32_t>();
	}
	
	const char *GetObjectName(uint16_t index)
	{
		auto &entry = GetEntry(index);
		
		if (!entry.name)
			return "unknown";
		
		return entry.name;
	}
};

void TomlTest(void);
ActorDatabase TomlLoadActorDatabase(const char *tomlPath);
ObjectDatabase TomlLoadObjectDatabase(const char *tomlPath);
void TomlInjectDataFromProject(Project *project, ActorDatabase *actorDb, ObjectDatabase *objectDb);
ActorDatabase::Entry TomlLoadActorDatabaseEntry(const char *tomlPath);
