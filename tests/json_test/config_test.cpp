#include <iostream>
#include <vector>
#include <list>

#include <nlohmann/json.hpp>
#include <configure_file.hpp>

int main()
{
	configuration_file file;
	configuration_file::json json1;
	json1["pi"] = 3.141;
	json1["happy"] = true;
	json1["extra"] = false;
	std::vector<uint8_t> vector;
	vector.push_back(1);vector.push_back(2);vector.push_back(3);
	json1["vector_test"] = vector;
	file.SetDefaultConfiguration(json1);
	auto ret = file.LoadConfiguration("config.json");
	
	auto target = file.get< std::vector<uint8_t> >("vector_test");
	
	std::cout << ret << target->size() << std::endl;
}