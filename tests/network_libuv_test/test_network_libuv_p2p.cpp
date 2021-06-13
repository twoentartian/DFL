#include <iostream>
#include <network.hpp>

std::string peer_callback(const char* data, int size)
{
	std::string str(data, size);
	std::cout << "receive: " << str << std::endl;
	return str;
}

int main(int argc, char **args)
{

	network::p2p peers[2];

	//peers[0].set_receive_callback()
	
	peers[0].set_receive_callback(peer_callback);
	peers[1].set_receive_callback(peer_callback);
	std::string data = "01234567890123456789";
	
	//test connect success
	{
		peers[1].start_service(1551);
		for (int i = 0; i < 100000000; ++i)
		{
			peers[0].send("127.0.0.1", 1551, network::i_p2p_node::ipv4, data.data(), data.size(), [](network::i_p2p_node::send_packet_status status, const char* data, size_t size){
				if (status == network::i_p2p_node::send_packet_success)
				{
					std::cout << "test pass" << std::endl;
				}
				else
				{
					std::cerr << "test fail" << std::endl;
				}
			});
		}
		peers[1].stop_service();
	}
	
	//test connect but server close
	{
		peers[0].send("127.0.0.1", 1551, network::i_p2p_node::ipv4, data.data(), data.size(), [](network::i_p2p_node::send_packet_status status, const char* data, size_t size){
			if (status == network::i_p2p_node::send_packet_success)
			{
				std::cerr << "test fail" << std::endl;
			}
			else
			{
				std::cout << "test pass" << std::endl;
			}
		});
	}
	
	std::cout << "press any key to exit" << std::endl;
	std::cin.get();
	
	peers[0].stop_service();
	peers[1].stop_service();
	return 0;
}
