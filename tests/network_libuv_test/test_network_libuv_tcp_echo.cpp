#include <iostream>
#include <network.hpp>

int main(int argc, char** args)
{
	network::tcp_server server;
	server.setMessageCallback([](uv::TcpConnectionPtr ptr,const char* data, ssize_t size)
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
			                                char data[20] = "hello world!";
			                                client.write(data, sizeof(data));
		                                }
		                                else
		                                {
			                                std::cout << "Error : connect to server fail" << std::endl;
		                                }
	                                });
	client.setMessageCallback([&client](const char* data,ssize_t size)
	                           {
		                           client.write(data,(unsigned)size,nullptr);
	                           });
	client.connect(addr);
	client.start();

	//press to exit
	std::cin.get();
	
	client.close();
	server.close();
	
	return 0;
}
