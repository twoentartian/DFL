#include <iostream>
#include <uv/include/uv11.hpp>

void startClients(uv::EventLoop* loop,uv::SocketAddr& addr,std::vector<uv::TcpClientPtr>& clients)
{
	uv::TcpClientPtr client = std::make_shared<uv::TcpClient>(loop);
	client->setConnectStatusCallback(
			[client](uv::TcpClient::ConnectStatus status)
			{
				if (status == uv::TcpClient::ConnectStatus::OnConnectSuccess)
				{
					char data[20] = "hello world!";
					client->write(data, sizeof(data));
				}
				else
				{
					std::cout << "Error : connect to server fail" << std::endl;
				}
			});
	client->setMessageCallback([client](const char* data,ssize_t size)
	{
		client->write(data,(unsigned)size,nullptr);
	});

	client->connect(addr);
	clients.push_back(client);
}

int main(int argc, char** args)
{
	uv::EventLoop* loop = uv::EventLoop::DefaultLoop();

	uv::TcpServer server(loop);
	server.setMessageCallback([&loop](uv::TcpConnectionPtr ptr,const char* data, ssize_t size)
	{
		std::cout << "server receive: " << std::string(data, size) << std::endl;
		loop->runInThisLoop([&loop](){
			loop->stop();
		});
	});

	//server.setTimeout(60); //heartbeat timeout.

	uv::SocketAddr addr("0.0.0.0", 10005, uv::SocketAddr::Ipv4);
	server.bindAndListen(addr);

	std::vector<uv::TcpClientPtr> clients;
	startClients(loop,addr,clients);

	std::thread t1([&loop](){
		loop->run();
	});
	std::thread t2([&loop](){
		loop->run();
	});
	t1.join();
	t2.join();
	
	return 0;
}
