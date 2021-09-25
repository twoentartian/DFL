#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

#include <boost/config.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/dll/alias.hpp>
#include <boost/dll/import.hpp>
#include <boost/function.hpp>

template<typename class_type>
class dll_loader
{
public:
	dll_loader() = default;
	dll_loader(const dll_loader& copy) = delete;
	
	std::tuple<bool, std::string> load(const std::string& dllPath, const std::string& class_name)
	{
		if (!std::filesystem::exists(dllPath))
		{
			return {false, "dll file not exist"};
		}
		_dll_path = dllPath;
		_class_name = class_name;
		boost::dll::fs::path libPath(_dll_path);
		try
		{
			_dll = boost::dll::import_symbol<class_type>(libPath, class_name);
		}
		catch (...)
		{
			return std::make_tuple(false, "fail to load function.");
		}
		
		return std::make_tuple(true, "success");
	}
	
	boost::shared_ptr<class_type> get()
	{
		return _dll;
	}
	
	const std::string& class_name()
	{
		return _class_name;
	}
	
	const std::string& dll_path()
	{
		return _dll_path;
	}

private:
	std::string _dll_path;
	std::string  _class_name;
	boost::shared_ptr<class_type> _dll;
};