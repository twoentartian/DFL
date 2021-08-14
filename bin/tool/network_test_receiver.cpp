#include <network.hpp>

int main()
{
	const std::string peer_ip = "192.168.0.114";
	const uint16_t peer_port = 6600;
	const uint16_t service_port = 6600;
	
	using namespace network;
	p2p_with_header _p2p_1;
	
	_p2p_1.set_receive_callback([](header::COMMAND_TYPE command, const char *data, int length) -> std::tuple<header::COMMAND_TYPE, std::string> {
		if (command == 0)
		{
			std::string data_str(data,length);
			std::cout << "receive data, size: " << data_str.size() << std::endl;
			return {0,"receive data, size: " + std::to_string(data_str.size())};
		}
		if (command == 1)
		{
			std::cout << "gets an error" << std::endl;
		}
		return {1,"error"};
	});
	_p2p_1.start_service(service_port, 4);
	
	std::cout << "press to exit" << std::endl;
	std::cin.get();
	_p2p_1.stop_service();
	
	return 0;
}