#pragma once

#include <string>

#include "./hash.hpp"


namespace crypto
{
	class sha256 : Hash
	{
	public:
		constexpr static int OutputSize = 256; //bits
		
		std::string digest(const std::string& message) override
		{
		
		}
		
	private:
	
	};
}
