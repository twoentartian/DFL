#pragma once

#include <iostream>

class command
{
public:
	static constexpr uint16_t unknown = 0;
	static constexpr uint16_t acknowledge = 1;
	static constexpr uint16_t acknowledge_but_not_accepted = 2;
	static constexpr uint16_t transaction = 3;
	static constexpr uint16_t block = 4;
	static constexpr uint16_t block_confirmation = 5;
	
	static constexpr uint16_t register_as_peer = 6;
	static constexpr uint16_t request_peer_info = 7;
	static constexpr uint16_t reply_peer_info = 8;
	
	
};