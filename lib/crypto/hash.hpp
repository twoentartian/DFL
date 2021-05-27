#pragma once
#include<cstdio>
#include <cstring>
#include <string>
#include <cstdint>

#include "./hex_data.hpp"

namespace crypto
{
	class Hash
	{
	public:
		constexpr static int OutputSize = 0;
		
		virtual hex_data digest(const std::string& message) = 0;
		
	};
	
	//sha256 md5 ecc
}
