//
// toml-parsers.hpp <z64scene>
//
// toml parsers live here, since
// it's relatively slow to compile
//

#include <stdio.h>
#include <stdint.h>

#include <vector>

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
		
		const char             *name;
		uint16_t                index;
		std::vector<uint16_t>   objects;
		std::vector<Property>   properties;
		
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
		
		entries[entry.index] = entry;
	}
	
	Entry &GetEntry(uint16_t index)
	{
		if (index >= entries.size())
		{
			static Entry empty;
			
			return empty;
		}
		
		return entries[index];
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
	struct ObjectDatabaseEntry
	{
	};
};

void TomlTest(void);
ActorDatabase TomlLoadActorDatabase(const char *tomlPath);
