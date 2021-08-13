#include <network.hpp>

int main()
{
	const std::string peer_ip = "192.168.1.109";
	const uint16_t peer_port = 6600;
	const uint16_t service_port = 6600;
	
	using namespace network;
	p2p_with_header _p2p_1;
	
	_p2p_1.set_receive_callback([](header::COMMAND_TYPE command, const char *data, int length) -> std::tuple<header::COMMAND_TYPE, std::string> {
		if (command == 0)
		{
			std::string data_str(data,length);
			std::cout << "receive data " << data_str << std::endl;
			return {0,data_str};
		}
		if (command == 1)
		{
			std::cout << "gets an error" << std::endl;
		}
		return {1,"error"};
	});
	_p2p_1.start_service(service_port, 4);
	
	std::string data_payload = "test+payload";
	
	bool _running = true;
	std::thread run_thread([&_running, &_p2p_1, &peer_ip, &peer_port, &data_payload](){
//		while (_running)
//		{
//			_p2p_1.send(peer_ip, peer_port, i_p2p_node_with_header::ipv4, 0, data_payload.data(), data_payload.length(), [](i_p2p_node_with_header::send_packet_status status, header::COMMAND_TYPE received_command, const char* data, int length)
//			{
//				std::cout << "send status: " << i_p2p_node_with_header::send_packet_status_message[status] << std::endl;
//			});
//			std::this_thread::sleep_for(std::chrono::milliseconds(200));
//		}
	});


	
//	p2p_with_header _p2p_2;
//	_p2p_2.set_receive_callback([](header::COMMAND_TYPE command, const char *data, int length) -> std::tuple<header::COMMAND_TYPE, std::string> {
//		std::this_thread::sleep_for(std::chrono::milliseconds(200));
//		if (command == 0)
//		{
//			std::string data_str(data,length);
//			std::cout << "receive data " << data_str << std::endl;
//			return {0,data_str};
//		}
//		if (command == 1)
//		{
//			std::cout << "gets an error" << std::endl;
//		}
//		return {1,"error"};
//	});
//	_p2p_2.start_service(service_port+1, 4);
//	_p2p_2.send("127.0.0.1", service_port, i_p2p_node_with_header::ipv4, 0, data_payload.data(), data_payload.length(), [](i_p2p_node_with_header::send_packet_status status, header::COMMAND_TYPE received_command, const char* data, int length)
//	{
//		std::cout << "send status: " << i_p2p_node_with_header::send_packet_status_message[status] << std::endl;
//	});
	
	std::cout << "press any key to exit" << std::endl;
	std::cin.get();
	
	_running = false;
	run_thread.join();
	_p2p_1.stop_service();
	
	return 0;
}