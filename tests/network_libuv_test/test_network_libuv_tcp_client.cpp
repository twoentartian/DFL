#include <iostream>
#include <network.hpp>


int main(int argc, char **args)
{
	uv::EventLoop *loop = uv::EventLoop::DefaultLoop();
	
	uv::TcpServer server(loop);
	server.setMessageCallback([&loop](uv::TcpConnectionPtr ptr, const char *data, ssize_t size)
	                          {
		                          std::cout << "server receive: " << std::string(data, size) << std::endl;
		                          loop->runInThisLoop([&loop]()
		                                              {
			                                              loop->stop();
		                                              });
	                          });
	
	//server.setTimeout(60); //heartbeat timeout.
	
	uv::SocketAddr addr("0.0.0.0", 10005, uv::SocketAddr::Ipv4);
	server.bindAndListen(addr);
	std::thread t1([&loop]()
	               {
		               loop->run();
	               });
	
	network::tcp_client client;
	client.setConnectStatusCallback(
			[&client](uv::TcpClient::ConnectStatus status)
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
	client.setMessageCallback([&client](const char *data, ssize_t size)
	                          {
		                          client.write(data, (unsigned) size, nullptr);
	                          });
	client.start();
	client.connect(addr);
	
	
	//press to stop
	std::cin.get();
	
	t1.join();
	client.close();
	
	return 0;
}
