#include <iostream>
#include <network.hpp>

network::tcp_server server;

void startClients(uv::EventLoop *loop, uv::SocketAddr &addr, std::vector<uv::TcpClientPtr> &clients)
{
	uv::TcpClientPtr client = std::make_shared<uv::TcpClient>(loop);
	client->setConnectStatusCallback(
			[client, loop](uv::TcpClient::ConnectStatus status)
			{
				if (status == uv::TcpClient::ConnectStatus::OnConnectSuccess)
				{
					char data[20] = "hello world!";
					client->write(data, sizeof(data));
					loop->runInThisLoop([loop]()
					                    {
						                    loop->stop();
					                    });
				} else
				{
					std::cout << "Error : connect to server fail" << std::endl;
				}
			});
	client->setMessageCallback([client](const char *data, ssize_t size)
	                           {
		                           client->write(data, (unsigned) size, nullptr);
	                           });
	
	client->connect(addr);
	clients.push_back(client);
}

int main(int argc, char **args)
{
	uv::EventLoop *loop = uv::EventLoop::DefaultLoop();
	
	server.setMessageCallback([&loop](uv::TcpConnectionPtr ptr, const char *data, ssize_t size)
	                          {
		                          std::cout << "server receive: " << std::string(data, size) << std::endl;
	                          });
	
	network::SocketAddr addr("0.0.0.0", 10005, uv::SocketAddr::Ipv4);
	int ret = server.bindAndListen(addr);
	if (ret != 0)
	{
		std::cout << "error to creat server" << std::endl;
		return 0;
	}
	server.start(1);
	
	std::vector<uv::TcpClientPtr> clients;
	startClients(loop, addr, clients);
	loop->run();
	
	std::cin.get();
	
	server.close();
	server.waitForClose();
	return 0;
}
