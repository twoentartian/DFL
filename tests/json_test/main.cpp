#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>

#include <nlohmann/json.hpp>
#include <configure_file.hpp>

BOOST_AUTO_TEST_SUITE (json_test)
	
BOOST_AUTO_TEST_CASE (json_test_basic)
{
	using json = nlohmann::json;
	json  json1;
	json1["pi"] = 3.141;
	json1["happy"] = true;
	std::string s = json1.dump();
	BOOST_CHECK(s == "{\"happy\":true,\"pi\":3.141}");
}

BOOST_AUTO_TEST_CASE (json_test_configuration)
{
	configuration_file file;
	configuration_file::json json1;
	json1["pi"] = 3.141;
	json1["happy"] = true;
	file.SetDefaultConfiguration(json1);
	file.LoadConfiguration("config.json");
	
}

BOOST_AUTO_TEST_SUITE_END()


