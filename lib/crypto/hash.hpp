#pragma once

namespace crypto
{
	class Hash
	{
	public:
		constexpr static int OutputSize = 0;
		
		virtual std::string digest(const std::string& message) = 0;
		
	};
}
