//
// toml-parsers.hpp <z64scene>
//
// toml parsers live here, since
// it's relatively slow to compile
//

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
	
	const char *GetActorName(uint16_t index)
	{
		if (index >= entries.size())
			return "unknown";
		
		auto entry = entries[index];
		
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
