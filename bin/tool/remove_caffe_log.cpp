#include <filesystem>
#include <iostream>

#include "remove_caffe_log.hpp"


int main()
{
	auto current_path = std::filesystem::current_path();
	auto log_path = current_path / "log";
	if (!std::filesystem::exists(log_path))
	{
		std::cout << "log folder does not find" << std::endl;
		return -1;
	}

	std::vector<std::string> caffe_keywords = {"caffe_solver_ext.hpp", "memory_data_layer.cpp"};
	for (auto& dir_entry: std::filesystem::directory_iterator{log_path})
	{
		if (dir_entry.is_regular_file() && !dir_entry.is_symlink())
		{
			std::cout << "find " << dir_entry.path().string() << ", begin processing.    ";

			auto [status, msg] = remove_caffe_log(dir_entry.path().string(), caffe_keywords);
			if (status)
			{
				std::cout << "status: " << msg << std::endl;
			}
			else
			{
				std::cout << "status: " << msg << std::endl;
			}
		}
	}
	
	return 0;
}