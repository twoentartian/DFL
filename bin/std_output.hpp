#pragma once
#include <mutex>
#include <iostream>

namespace std_cout
{
	std::mutex lock;
	void print(std::string message)
	{
		std::lock_guard guard(lock);
		std::cout << message;
	}
	
	void println(std::string message)
	{
		std::lock_guard guard(lock);
		std::cout << message << std::endl;
	}
}

namespace std_cerr
{
	std::mutex lock;
	void print(std::string message)
	{
		std::lock_guard guard(lock);
		std::cerr << message;
	}
	
	void println(std::string message)
	{
		std::lock_guard guard(lock);
		std::cerr << message << std::endl;
	}
}