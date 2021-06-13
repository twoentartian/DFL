#define DISABLE_NETWORK_BUILTIN_LOG

#include <iostream>
#include <network.hpp>

int main(int argc, char **args)
{
	for (int i = 0; i < 10000; ++i)
	{
		network::tcp_server server;
		server.setMessageCallback([](uv::TcpConnectionPtr ptr, const char *data, ssize_t size)
		                          {
			                          std::cout << "server receive: " << std::string(data, size) << std::endl;
		                          });
		uv::SocketAddr addr("0.0.0.0", 10005, uv::SocketAddr::Ipv4);
		server.bindAndListen(addr);
		server.start();
		
		network::tcp_client client;
		client.setConnectStatusCallback([&client](uv::TcpClient::ConnectStatus status)
		                                {
			                                if (status == uv::TcpClient::ConnectStatus::OnConnectSuccess)
			                                {
				                                std::cout << "connection start" << std::endl;
				                                std::string data =  "hello world!";
				                                client.write(data.data(), data.size());
			                                }
			                                else
			                                {
				                                std::cout << "connection stopped" << std::endl;
			                                }
		                                });
		client.setMessageCallback([&client](const char *data, ssize_t size)
		                          {
			                          std::cout << "receive data: " << size << std::endl;
			                          client.write(data, (unsigned) size, nullptr);
		                          });
		
		client.connect(addr);
		client.start();
		
		//press to close connection
		//std::cin.get();
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		
		auto connections = server.getAllConnection();
		for (auto&& single_connection : connections)
		{
			server.closeConnection(single_connection.first);
		}
		server.close();
	}
	
	
	return 0;
}
