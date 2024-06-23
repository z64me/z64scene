//
// toml-parsers.hpp <z64scene>
//
// toml parsers live here, since
// it's relatively slow to compile
//

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <vector>
#include <string>
#include <map>

extern "C" {
#include "project.h"
}

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
		};
		
		char                   *name;
		uint16_t                index;
		std::vector<uint16_t>   objects;
		std::vector<Property>   properties;
		bool isEmpty = false;
		int categoryInt = -1;
		
		void AddProperty(Property property)
		{
			properties.emplace_back(property);
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
		
		tmp.name = 0;
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
