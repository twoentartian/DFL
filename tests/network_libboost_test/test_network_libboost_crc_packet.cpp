//
// Created by gzr on 24-06-21.

#include <thread>
#include <iostream>
#include <network-common/packet_header.hpp>
#include "../../bin/transaction.hpp"
#include <crypto.hpp>
#include <hash_interface.hpp>
#include <boost_serialization_wrapper.hpp>
#include <glog/logging.h>

int main()
{
	constexpr int size = 10*1000;
	std::string data = util::get_random_str(size);
	
	network::packet_header header(1, data.size());
	std::stringstream ss;
	ss << header.get_header_byte() << data;

	network::header_decoder decoder;
	bool wait = true;
	decoder.set_receive_callback([&data, &wait](uint16_t command, std::shared_ptr<std::string> received_data){
		if (data == *received_data)
		{
			std::cout << "pass" << std::endl;
		}
		else
		{
			std::cout << "fail" << std::endl;
		}
		wait = false;
	});
	
	std::string write_data = ss.str();
	for (int i = 0; i < 9; ++i)
	{
		decoder.receive_data(write_data.substr(i*1000, 1000));
	}
	decoder.receive_data(write_data.substr(9*1000));
	
	while (wait)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	
    return 0;
}