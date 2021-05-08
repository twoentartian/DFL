#pragma once

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
