#define NETWORK_USE_LIBBOOST
#include "network.hpp"

int main()
{
	std::string self_ip = network::local_address();
	std::cout << self_ip << std::endl;
	
	network::udp::udp_socket udp0(1000);
	network::udp::udp_socket udp1(1001);
	
	udp0.SetReceiveHandler([&udp0](std::shared_ptr<boost::asio::ip::udp::endpoint> remote_ep, uint8_t* data, std::size_t size){
		std::string data_str(reinterpret_cast<char*>(data), size);
		udp0.Send(*remote_ep, data_str);
	});
	
	udp0.SetReceiveHandler([&udp0](std::shared_ptr<boost::asio::ip::udp::endpoint> remote_ep, uint8_t* data, std::size_t size){
		std::string data_str(reinterpret_cast<char*>(data), size);
		std::cout << "udp0: " << data_str << std::endl;
	});
	udp1.SetReceiveHandler([&udp0](std::shared_ptr<boost::asio::ip::udp::endpoint> remote_ep, uint8_t* data, std::size_t size){
		std::string data_str(reinterpret_cast<char*>(data), size);
		std::cout << "udp1: " << data_str << std::endl;
	});
	
	auto status0 = udp0.Open(1005);
	auto status1 = udp1.Open(1006);
	
	std::string data = "01234567890123456789";
	auto [ProtocolStatus, send_size] = udp1.Send("127.0.0.1", 1000, data);
	
	std::cin.get();
	
}