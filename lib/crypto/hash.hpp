#pragma once
#include<stdio.h>
#include <string.h>
#include <string>
#include <stdint.h>
namespace crypto
{
	class Hash
	{
	public:
		constexpr static int OutputSize = 0;
		
		virtual std::string digest(const std::string& message) = 0;
		
	};
	
	//sha256 md5 ecc
}
