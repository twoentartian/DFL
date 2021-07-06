#pragma once
#include<cstdio>
#include <cstring>
#include <string>
#include <cstdint>

#include "./crypto_config.hpp"
#include "./hex_data.hpp"
#include <byte_buffer.hpp>
#include <exception.hpp>

namespace crypto
{
	class hash
	{
	public:
		constexpr static int OutputSize = 0;
		
		virtual hex_data digest(const std::string& message) = 0;

		virtual hex_data digest(const uint8_t* data, size_t size) = 0;
		
#if USE_OPENSSL
		virtual hex_data digest_openssl(const std::string& message)
		{
			THROW_NOT_IMPLEMENTED;
		}
#endif
	};
}
