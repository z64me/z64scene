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
}

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
			
			const char *QuickSanitizedName(void)
			{
				static char work[256];
				const char *src = name;
				for (char *dst = work; ; ++dst, ++src)
				{
					*dst = *src;
					if (!*dst)
						break;
					if (!isalnum(*dst))
						--dst;
				}
				return work;
			}
		};
		
		char                   *name;
		uint16_t                index;
		std::vector<uint16_t>   objects;
		std::vector<Property>   properties;
		bool isEmpty = false;
		int categoryInt = -1;
		char *rendercode = 0;
		uint32_t rendercodeLineNumberInToml = 0;
		uint32_t rendercodeLineNumberOffset = 0;
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
			if (!rendercode)
				return 0;
			
			static char *work = 0;
			
			if (!work)
				work = (char*)malloc(4096);
			
			char *buf = work;
			*buf = '\0';
			STRCATF(buf, "%s", R"(
				class World {
					foreign static Xpos
					foreign static Ypos
					foreign static Zpos
				}
				class Draw {
					foreign static SetScale(xscale, yscale, zscale)
					foreign static SetScale(scale)
				}
			)");
			
			// properties
			STRCATF(buf, "%s", R"(
				class props__hooks {
			)");
			for (auto &option : properties) {
				const char *name = option.QuickSanitizedName();
				STRCATF(buf, "%s { _%s }\n", name, name);
				STRCATF(buf, "%s=(v) { _%s = v }\n", name, name);
			}
			STRCATF(buf, "%s", R"(
				construct new() { }
				}
				var Props = props__hooks.new()
			)");
			
			// constants
			for (int i = 0; i < objects.size(); ++i)
			{
				auto symbolAddresses = GetObjectSymbolAddresses(objects[i]);
				
				for (const auto &sym : symbolAddresses)
					STRCATF(buf, "var %s = 0x%08x\n", sym.first.c_str(), sym.second);
			}
			
			// render code offset
			rendercodeLineNumberOffset = 1;
			for (char *tmp = work; *tmp; ++tmp)
				if (*tmp == '\n')
					rendercodeLineNumberOffset += 1;
			
			// rendercode
			STRCATF(buf, "class hooks { static draw() { \n %s \n } } ", rendercode);
			
			return work;
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
		auto tmp = GetEntry(index);
		
		if (tmp.name)
			free(tmp.name);
		
		if (tmp.rendercode)
			free(tmp.rendercode);
		
		tmp.name = 0;
		tmp.rendercode = 0;
		tmp.isEmpty = true;
		tmp.objects = std::vector<uint16_t>();
		tmp.properties = std::vector<Entry::Property>();
	}
	
	const char *GetActorName(uint16_t index)
	{
		auto entry = GetEntry(index);
		
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
		uint16_t index;
		std::map<std::string, uint32_t> symbolAddresses;
		bool isEmpty = false;
		
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
				fprintf(stderr, "'%s' doesn't exist\n", symsPath);
			
			// consider these consumed
			free(symsPath);
			symsPath = 0;
		}
		
		void PrintAllSyms(void)
		{
			// print contents
			for (const auto &sym : symbolAddresses)
				fprintf(stderr, "%s = 0x%08x;\n", sym.first.c_str(), sym.second);
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
		auto tmp = GetEntry(index);
		
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
		auto entry = GetEntry(index);
		
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
