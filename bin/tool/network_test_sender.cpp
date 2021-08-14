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
	data_payload.resize(5*1000*1000);
	
	std::condition_variable cv;
	std::mutex lock;
	std::unique_lock uniqueLock(lock);
	bool _running = true;
	std::thread run_thread([&_running, &_p2p_1, &peer_ip, &peer_port, &data_payload, &uniqueLock, &cv](){
		while (_running)
		{
			_p2p_1.send(peer_ip, peer_port, i_p2p_node_with_header::ipv4, 0, data_payload.data(), data_payload.length(), [](i_p2p_node_with_header::send_packet_status status, header::COMMAND_TYPE received_command, const char* data, int length)
			{
				std::cout << "send status: " << i_p2p_node_with_header::send_packet_status_message[status] << std::endl;
			});
			std::cout<< "loop" << std::endl;
			cv.wait(uniqueLock);
		}
	});
	
	
	std::cout << "press q to exit" << std::endl;
	while (true)
	{
		char c = std::cin.get();
		if (c == 'q')
			break;
		else
			cv.notify_all();
	}
	
	_running = false;
	cv.notify_all();
	run_thread.join();
	_p2p_1.stop_service();
	
	return 0;
}