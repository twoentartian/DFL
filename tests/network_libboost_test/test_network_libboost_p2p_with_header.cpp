#define NETWORK_USE_LIBBOOST
#include <iostream>
#include <network.hpp>

std::tuple<network::header::COMMAND_TYPE, std::string> peer_callback(network::header::COMMAND_TYPE command, const char* data, int size, std::string ip)
{
	static int counter = 0;
	std::string str(data, size);
	std::cout << counter << " receive: " << str << std::endl;
	counter++;
	return {1,str};
}

int main(int argc, char **args)
{
	const int port = 1523;
	network::p2p_with_header peers[2];
	
	//peers[0].set_receive_callback()
	
	peers[0].set_receive_callback(peer_callback);
	peers[1].set_receive_callback(peer_callback);
	std::string data;
	data.resize(1000*1000);
	for (auto& c: data)
	{
		c = '0';
	}
	
	//test connect success
	{
		peers[1].start_service(port);
		for (int i = 0; i < 1000; ++i)
		{
			peers[0].send("127.0.0.1", port, network::i_p2p_node_with_header::ipv4, 1, data.data(), data.size(), [](network::i_p2p_node_with_header::send_packet_status status, network::header::COMMAND_TYPE command, const char* data, size_t size) {
				if (status == network::i_p2p_node_with_header::send_packet_success)
				{
					std::cout << "test pass" << std::endl;
				}
				else
				{
					std::cout << "test fail: " << status << std::endl;
				}
			});
		}
	}
	
	std::cout << "press any key to exit" << std::endl;
	std::cin.get();
	
	peers[0].stop_service();
	peers[1].stop_service();
	return 0;
}
