#pragma once

#include <tuple>
#include <filesystem>
#include <fstream>
#include <vector>

std::tuple<bool, std::string> remove_caffe_log(const std::string& path, const std::vector<std::string>& caffe_keywords)
{
	if (!std::filesystem::exists(path))
	{
		return {false, "file not exist"};
	}
	if (path.find(".nocaffe.log") != std::string::npos)
	{
		return {false, "processed file and will be ignored"};
	}
	
	std::filesystem::path input_path(path);
	std::filesystem::path output_path(path + ".nocaffe.log");
	
	std::ifstream input_file; input_file.open(input_path, std::ios::binary);
	std::ofstream output_file; output_file.open(output_path, std::ios::binary);
	
	if (!input_file) return {false, "cannot read input file"};
	if (!output_file) return {false, "cannot write to output file"};
	
	std::string single_line;
	while(std::getline(input_file, single_line))
	{
		bool is_caffe = false;
		for (auto& keywords: caffe_keywords)
		{
			if (single_line.find(keywords) != std::string::npos)
			{
				is_caffe = true;
				break;
			}
		}
		
		if (!is_caffe)
		{
			output_file << single_line << std::endl;
		}
	}
	
	output_file.flush();
	output_file.close();
	input_file.close();
	
	return {true, "success"};
}