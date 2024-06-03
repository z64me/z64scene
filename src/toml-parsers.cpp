//
// toml-parsers.cpp <z64scene>
//
// toml parsers live here, since
// it's relatively slow to compile
//

// standard stuff
#include <iostream>
#include <cstring>

// toml stuff
#include <toml.hpp>

// toml helpers
#define TOML_CSTRING(X) strdup(std::string(X.as_string()).c_str())
#define TOML_INT(X) X.as_integer()

// json stuff
#if 0
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#endif

#include "toml-parsers.hpp"

static ActorDatabase::Entry::Property ActorDatabasePropertyFromToml(toml::value property)
{
	ActorDatabase::Entry::Property result;
	
	result.mask = TOML_INT(property["Mask"]);
	result.name = TOML_CSTRING(property["Name"]);
	result.target = TOML_CSTRING(property["Target"]);
	
	auto dropdown = property["Dropdown"];
	
	if (!dropdown.is_array())
		return result;
	
	for (auto &each : dropdown.as_array())
	{
		result.AddCombo(TOML_INT(each[0]), TOML_CSTRING(each[1]));
	}
	
	return result;
}

static std::vector<ActorDatabase::Entry::Property> ActorDatabasePropertiesFromToml(toml::value properties)
{
	std::vector<ActorDatabase::Entry::Property> result;
	
	if (!properties.is_array())
		return result;
	
	for (auto &property : properties.as_array())
	{
		result.emplace_back(ActorDatabasePropertyFromToml(property));
	}
	
	return result;
}

static ActorDatabase::Entry ActorDatabaseEntryFromToml(toml::value tomlActor)
{
	ActorDatabase::Entry entry;
	
	entry.name = TOML_CSTRING(tomlActor["Name"]);
	entry.index = TOML_INT(tomlActor["Index"]);
	entry.object = TOML_INT(tomlActor["Object"]);
	entry.properties = ActorDatabasePropertiesFromToml(tomlActor["Property"]);
	
	return entry;
}

// testing toml code
void TomlTest(void)
{
	std::cout << "TomlTest():" << std::endl;
	std::string tomlData = R"(
		Title = "Example"
		[[Actor]]
			Name = "Stalfos"
			Index = 0x0002
			Object = 0x0032
			BinaryTest = 0b0011
			
			[[Actor.Property]]
				Mask          = 1
				Name          = "Set Flag"
				Target        = "XRot"
				Dropdown      = [ [ 0xFF, "NULL"] ]
			[[Actor.Property]]
				Mask           = 2
				Name           = "Get Flag"
				Target         = "XRot"
				Dropdown       = [ [ 0xFF, "NULL"] ]
		
		[[Actor]]
			Name = "Wolfos"
			Index = 0x01AF
			Object = 0x0183
			BinaryTest = 0b0011
			
			[[Actor.Property]]
				Mask          = 3
				Name          = "Set Flag"
				Target        = "XRot"
				Dropdown      = [ [ 0xFF, "NULL"] ]
			[[Actor.Property]]
				Mask           = 0b0100
				Name           = "Get Flag"
				Target         = "XRot"
				Dropdown       = [ [ 0xFF, "NULL"] ]
		
		[[Actor]]
			Name = "Test"
			Index = 0x1234
			Object = 0x5678
			
			[[Actor.Property]]
				Mask          = 0xFF00
				Name          = "Set Flag"
				Target        = "XRot"
				Dropdown = [ [ 0xFF, "NULL"] ]
			[[Actor.Property]]
				Mask           = 0x00FF
				Name           = "Get Flag"
				Target         = "XRot"
				Dropdown = [ [ 0xFF, "NULL"] ]
			[[Actor.Property]]
				Mask          = 0xC000
				Name          = "Set Flag Type"
				Target        = "Var"
				Dropdown = [
					[ 0, "Switch"      ],
					[ 1, "Chest"       ],
					[ 2, "Collectible" ],
					[ 3, "Global"      ],
				]
			[[Actor.Property]]
				Mask          = 0x3000
				Name          = "Get Flag Type"
				Target        = "Var"
				Dropdown = [
					[ 0, "Switch"      ],
					[ 1, "Chest"       ],
					[ 2, "Collectible" ],
					[ 3, "Global"      ],
				]
			[[Actor.Property]]
				Mask          = 0xFF00
				Name          = "Flip Get"
				Target        = "YRot"
				Dropdown = [ [ 0, "false"], [ 1, "true" ] ]
			[[Actor.Property]]
				Mask          = 0x0C00
				Name          = "Force"
				Target        = "Var"
				Dropdown = [ [ 0, "false"], [ 1, "true" ] ]
			[[Actor.Property]]
				Mask          = 0x03FF
				Name          = "Distance x10"
				Target        = "Var"
			[[Actor.Property]]
				Mask          = 0xF0
				Name          = "Distance Type"
				Target        = "YRot"
				Dropdown = [ [ 0, "XZ" ], [ 1, "XZY" ] ]
			[[Actor.Property]]
				Mask          = 0xF
				Name          = "Header Index"
				Target        = "YRot"
		
		[[Actor]]
			Name = "Test w/o Properties"
			Index = 0x0001
			Object = 0x5678
	)";
	std::stringstream test(tomlData);
	auto data = toml::parse(test);
	
	/*
	std::cout << data["title"] << std::endl;
	std::cout << data["actor"] << std::endl;
	std::cout << data["actor"]["name"] << std::endl;
	std::cout << data["actor"]["binarytest"] << std::endl;
	*/
	//std::cout << data["Property"] << std::endl;
	//std::cout << data << std::endl;
	
	auto tomlActors = data["Actor"].as_array();
	
	ActorDatabase actorDatabase;
	
	// load actor database
	for (auto &tomlActor : tomlActors)
	{
		auto actor = ActorDatabaseEntryFromToml(tomlActor);
		
		actorDatabase.AddEntry(actor);
	}
	
	// display actor database
	for (auto &actor : actorDatabase.entries)
	{
		if (!actor.name)
			continue;
		
		fprintf(stderr, "%s : %04x %04x\n", actor.name, actor.index, actor.object);
		
		for (auto &prop : actor.properties)
		{
			fprintf(stderr, "  %s in %s\n", prop.name, prop.target);
			
			for (auto &combo : prop.combos)
				fprintf(stderr, "     %s = %d\n", combo.label, combo.value);
		}
	}
}

// json stuff
#if 0
// testing code from the nlohmann/json readme
void JsonTest(void)
{
	json j = json::parse(R"(
		{
			"pi": 3.141,
			"happy": true
		}
	)");
	
	std::cout << j.dump(4) << std::endl;
	
	fprintf(stdout, "%f\n", (float)j["pi"]); // casting is necessary
	std::cout << j["pi"] << std::endl;
	
	fprintf(stdout, "----\n");
	
	json j2 = {
		{"pi", 3.141},
		{"happy", true},
		{"name", "Niels"},
		{"nothing", nullptr},
		{"answer", {
			{"everything", 42}
		}},
		{"list", {1, 0, 2}},
		{"object", {
			{"currency", "USD"},
			{"value", 42.99}
		}}
	};
	
	fprintf(stdout, "%f\n", (float)j2["pi"]);
	std::cout << j2["pi"] << std::endl;
	std::cout << j2.dump(4) << std::endl;
}
#endif