#include <iostream>
#include <boost/filesystem.hpp>

int main()
{
	std::cout << "Hello, World!" << std::endl;
	std::cout << boost::filesystem::exists(".");
	
	return 0;
}
