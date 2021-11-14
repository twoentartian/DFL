#pragma once

#include <configure_file.hpp>

configuration_file::json get_default_introducer_configuration()
{
	configuration_file::json output;
	
	output["port"] = 5666;
	output["blockchain_public_key"] = "070a95eb6bd64eb273a2649a0ea61b932d50417275b8e250595c7a47468534a86fbd01f2f06efbd226ef86ab56abdd5cc296800794c9250c12c105fa3df86d3ad7";
	output["blockchain_private_key"] = "e321c369d8b1ff39aeabadc7846735e4fe5178af41b87eb421a6773c8c4810c3";
	output["blockchain_address"] = "a49487e4550b9961af17161900c8496a4ddc0638979d3c4f9b305d2b4dc0c665";
	
	return output;
}