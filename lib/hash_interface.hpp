#pragma once

#include "crypto.hpp"

class i_hashable
{
public:
	virtual void to_byte_buffer(byte_buffer&) = 0;
	
};