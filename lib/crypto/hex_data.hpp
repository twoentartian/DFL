#pragma once

#include <vector>
#include <string>
#include <cstring>
#include <stdexcept>
#include <algorithm>

#include <util.hpp>

namespace crypto
{
	class hex_data
	{
	public:
		hex_data(const std::string& binary_data)
		{
			data_str = binary_data;
			data_memory = ConvertTextToHex(binary_data);
		}
		
		hex_data(const uint8_t* data, size_t size)
		{
			data_memory.resize(size);
			std::memcpy(data_memory.data(), data, size * sizeof(uint8_t));
			data_str = ConvertHexToText(data, size);
		}
		
		hex_data(const char* data, size_t size) : hex_data(reinterpret_cast<const uint8_t*>(data), size){}
		
		hex_data(const std::vector<uint8_t>& memory_data) : hex_data(memory_data.data(), memory_data.size()){}
		
		hex_data(const std::vector<char>& memory_data) : hex_data(memory_data.data(), memory_data.size()){}
		
		GENERATE_GET(data_str, getTextStr_uppercase);
		GENERATE_GET(data_memory, getHexMemory);
		
		std::string getTextStr_lowercase()
		{
			std::string output;
			output.resize(data_str.size());
			std::transform(data_str.begin(), data_str.end(), output.begin(),[](unsigned char c)
			{
				return std::tolower(c);
			});
			return output;
		}
		
	private:
		std::string data_str;
		std::vector<uint8_t> data_memory;
		
		std::string ConvertHexToText(const uint8_t*  data, size_t length)
		{
			char* text = new char[length * 2 + 1];
			for (uint32_t i = 0; i < length; i++)
			{
				text[i * 2] = HexToText(data[i] / 16);
				text[i * 2 + 1] = HexToText(data[i] % 16);
			}
			text[length * 2] = '\0';
			std::string str(text);
			delete[] text;
			return str;
		}
		
		std::vector<uint8_t> ConvertTextToHex(const std::string& text)
		{
			std::vector<uint8_t> output;
			output.reserve(text.size() * 2);
			for (int8_t loc = 0; loc < text.size(); loc = loc +2)
			{
				output.push_back(TextToHex(text[loc], text[loc + 1]));
			}
			return output;
		}
		
		int8_t HexToText(uint8_t value)
		{
			if (value < 10)
			{
				return '0' + value;
			}
			else
			{
				return 'A' + value - 10;
			}
		}
		
		uint8_t TextToHex(int8_t inputHigh, int8_t inputLow)
		{
			uint8_t output = 0;
			
			uint8_t value = 0;
			if (inputHigh >= 'A' && inputHigh <= 'Z')
			{
				value = inputHigh - 'A' + 10;
			}
			else if(inputHigh >= 'a' && inputHigh <= 'z')
			{
				value = inputHigh - 'a' + 10;
			}
			else if(inputHigh >= '0' && inputHigh <= '9')
			{
				value = inputHigh - '0';
			}
			else
			{
				//ERROR
				throw std::logic_error("Not accepted value");
			}
			output += 16 * value;
			value = 0;
			if (inputLow >= 'A' && inputLow <= 'Z')
			{
				value = inputLow - 'A' + 10;
			}
			else if (inputLow >= 'a' && inputLow <= 'z')
			{
				value = inputLow - 'a' + 10;
			}
			else if (inputLow >= '0' && inputLow <= '9')
			{
				value = inputLow - '0';
			}
			else
			{
				//ERROR
				throw std::logic_error("Not accepted value");
			}
			output += value;
			
			return output;
		}
	};
}