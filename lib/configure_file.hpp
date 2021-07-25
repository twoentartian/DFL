#pragma once

#include <filesystem>
#include <fstream>
#include <optional>
#include <nlohmann/json.hpp>

class configuration_file
{
public:
	using json = nlohmann::json;
	
	enum configuration_file_return_code
	{
		InvalidPath = -4,
		DefaultConfigNotProvided = -3,
		FileNotFound = -2,
		FileFormatError = -1,
		NoError = 0,
		FileNotFoundAndGenerateOne = 1,
	};
	
	void SetDefaultConfiguration(const nlohmann::json& json)
	{
		_defaultConfiguration = json;
	}
	
	configuration_file_return_code LoadConfigurationData(const std::string& fileData)
	{
		try
		{
			_currentConfiguration = json::parse(fileData);
		}
		catch (...)
		{
			return FileFormatError;
		}
		return NoError;
	}
	
	configuration_file_return_code LoadConfiguration(const std::string& filePath, bool generate_if_not_exist = true, bool over_write_if_error = false)
	{
		_current_configuration_path = filePath;
		if (!std::filesystem::exists(filePath))
		{
			if (generate_if_not_exist)
			{
				if (_defaultConfiguration.empty())
				{
					return DefaultConfigNotProvided;
				}
				
				//write default config
				auto ret = write_config(_current_configuration_path, _defaultConfiguration);
				if (ret != NoError) return ret;
				
				_currentConfiguration = _defaultConfiguration;
				return FileNotFoundAndGenerateOne;
			}
			else
			{
				return FileNotFound;
			}
		}
		std::string content;
		{
			//read config
			std::ifstream ifs(filePath, std::ios::binary);
			if (!ifs.is_open()) return InvalidPath;
			content.assign((std::istreambuf_iterator<char>(ifs)),(std::istreambuf_iterator<char>()));
			ifs.close();
		}
		
		try
		{
			_currentConfiguration = json::parse(content);
		}
		catch (...)
		{
			if (over_write_if_error)
			{
				//write default config
				_currentConfiguration = _defaultConfiguration;
				auto ret = write_config(_current_configuration_path, _defaultConfiguration);
				if (ret != NoError) return ret;
				return NoError;
			}
			else
			{
				return FileFormatError;
			}
		}
		
		// merge configuration with default
		if (!_defaultConfiguration.empty())
		{
			bool is_missing_config = false;
			for (json::iterator it = _defaultConfiguration.begin(); it != _defaultConfiguration.end(); ++it)
			{
				if (!_currentConfiguration.contains(it.key()))
				{
					is_missing_config = true;
					_currentConfiguration[it.key()] = it.value();
				}
			}
			
			//write back config
			if (is_missing_config)
			{
				auto ret = write_config(_current_configuration_path, _currentConfiguration);
				if (ret != NoError) return ret;
			}
		}
		return NoError;
	}
	
	template<typename T>
	[[nodiscard]] std::optional<T> get(const std::string& key_name) const
	{
		T output;
		try
		{
			output = _currentConfiguration[key_name].get<T>();
		}
		catch (...)
		{
			return {};
		}
		return {output};
	}
	
	template<typename T>
	[[nodiscard]] std::optional<std::vector<T>> get_vec(const std::string& key_name) const
	{
		std::vector<T> output;
		configuration_file::json json_node;
		try
		{
			json_node = _currentConfiguration[key_name];
			for (auto iter = json_node.begin(); iter != json_node.end() ; iter++)
			{
				output.push_back(iter.value());
			}
		}
		catch (...)
		{
			return {};
		}
		return {output};
	}
	
	json& get_json()
	{
		return _currentConfiguration;
	}
	
	configuration_file_return_code write_back()
	{
		const std::string configuration_content = _currentConfiguration.dump(4);
		std::ofstream ofs;
		ofs.open(_current_configuration_path, std::ios::binary | std::ios::out);
		if (!ofs.is_open()) return InvalidPath;
		ofs << configuration_content;
		ofs.flush();
		ofs.close();
		return NoError;
	}

private:
	std::string _current_configuration_path;
	json _defaultConfiguration;
	json _currentConfiguration;
	
	configuration_file_return_code write_config(const std::string& path, const json& json)
	{
		const std::string configuration_content = json.dump(4);
		std::ofstream ofs;
		ofs.open(path, std::ios::binary | std::ios::out);
		if (!ofs.is_open()) return InvalidPath;
		ofs << configuration_content;
		ofs.flush();
		ofs.close();
		return NoError;
	}
};