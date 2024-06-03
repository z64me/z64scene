//
// toml-parsers.cpp <z64scene>
//
// toml parsers live here, since
// it's relatively slow to compile
//

// standard stuff
#include <iostream>

// toml stuff
#include <toml.hpp>

// json stuff
#if 0
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#endif

#include "toml-parsers.hpp"

// testing toml code
void TomlTest(void)
{
	std::cout << "TomlTest():" << std::endl;
	std::string tomlData = R"(
		Title = "Example"
		[[Actor]]
			Name = "Stalfos"
			Number = 0x0002
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
			Number = 0x01AF
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
	
	auto actors = data["Actor"];
	if (actors.is_array()) std::cout << "is array" << std::endl;
	for (auto &actor : actors.as_array())
	{
		//std::cout << "cool" << std::endl;
		std::cout << actor["Name"] << std::endl;
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
