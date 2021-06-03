#include <dll_importer.hpp>
#include "dll_interface.hpp"

int main()
{
	{
		dll_loader<interface> dll_loader;
		dll_loader.load("libTEST_dll_dll_target.so", "plugin");
		std::cout << dll_loader.get()->calculate(1,2) << std::endl;
	}
	
	return 0;
}
