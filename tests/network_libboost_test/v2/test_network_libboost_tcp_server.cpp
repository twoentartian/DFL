#define NETWORK_USE_LIBBOOST

#include <boost/format.hpp>
#include <boost/shared_array.hpp>
#include "network.hpp"

std::atomic_int temp1 = 0, temp2 = 0, temp3 = 0, temp4 = 0;

int main()
{
	using namespace network;
	constexpr int PORT = 1500;
	constexpr uint32_t dataLength = 10 * 1024;
	
	std::recursive_mutex cout_lock;
	network::simple::tcp_server server;
	
	server.SetAcceptHandler([&cout_lock](const std::string &ip, uint16_t port, std::shared_ptr<simple::tcp_session> session)
	                        {
		                        std::lock_guard<std::recursive_mutex> temp_lock_guard(cout_lock);
		                        std::cout << "[server] accept: " << ip << ":" << port << std::endl;
		                        temp1++;
	                        });
	server.SetSessionReceiveHandler([&cout_lock](char *data, uint32_t length, std::shared_ptr<simple::tcp_session> session_receive)
	                                {
		                                temp2++;
		                                std::lock_guard<std::recursive_mutex> temp_lock_guard(cout_lock);
		                                std::cout << "[server] receive (ok) length " << length << std::endl;
		                                session_receive->write(data, length);
	                                });
	server.SetSessionCloseHandler([&](std::shared_ptr<simple::tcp_session> session_close)
	                              {
		                              std::lock_guard<std::recursive_mutex> temp_lock_guard(cout_lock);
		                              std::cout << "[server] close: " << session_close->ip() << ":" << session_close->port() << std::endl;
	                              });
	
	auto ret_code = server.Start(PORT, 6);
	std::cout << "server: " << std::to_string(ret_code) << std::endl;
	
	// tcp_client

	const int NumberOfClient = 1000;
	std::shared_ptr<tcp_client> clients[NumberOfClient];
	int client_counts[NumberOfClient];
	
	for (size_t i = 0; i < NumberOfClient; i++)
	{
		clients[i] = tcp_client::CreateClient();
		client_counts[i] = 0;
	}
	
	for (size_t i = 0; i < NumberOfClient; i++)
	{
		clients[i]->connect("127.0.0.1", PORT);
		clients[i]->SetConnectHandler([&cout_lock](tcp_status status, std::shared_ptr<tcp_client> client)
		                              {
			                              if (status == tcp_status::Success)
			                              {
				                              {
					                              std::lock_guard<std::recursive_mutex> temp_lock_guard(cout_lock);
					                              std::cout << "[client] connect:" << client->ip() << ":" << client->port() << std::endl;
				                              }
				                              uint8_t *data = new uint8_t[dataLength];
				                              for (size_t i = 0; i < dataLength; i++)
				                              {
					                              data[i] = uint8_t(i);
				                              }
				                              auto status = client->SendRaw(data, dataLength);
				                              if (status == network::Success)
				                              {
					                              temp3++;
				                              }
				                              else
				                              {
					                              temp4++;
				                              }
				
				                              delete[] data;
			                              }
			                              else
			                              {
				                              std::lock_guard<std::recursive_mutex> temp_lock_guard(cout_lock);
				                              std::cout << "[client] fail to connect:" << client->ip() << ":" << client->port() << std::endl;
			                              }
		                              });
		
		clients[i]->SetReceiveHandler([&cout_lock, i, &client_counts](uint8_t *data, uint32_t length, tcp_status status, std::shared_ptr<tcp_client> client)
		                              {
			                              if (status == tcp_status::Success)
			                              {
				                              {
					                              std::lock_guard<std::recursive_mutex> temp_lock_guard(cout_lock);
					                              std::cout << "[client] receive (ok) length " << length << " count: " << client_counts[i] << std::endl;
				                              }
				                              client_counts[i]++;
				                              if (client_counts[i] < 50)
				                              {
					                              client->SendRaw(data, dataLength);
				                              }
			                              }
			                              else
			                              {
				                              std::lock_guard<std::recursive_mutex> temp_lock_guard(cout_lock);
				                              std::cout << "[client] receive (protocol error) length " << length << std::endl;
			                              }
		                              });
		
		clients[i]->SetCloseHandler([&cout_lock](const std::string &ip, uint16_t port)
		                            {
			                            //std::lock_guard<std::recursive_mutex> temp_lock_guard(cout_lock);
			                            std::cout << "[client] connection close: " << ip << ":" << port
			                                      << std::endl;//NEVER Delete this line, because when you delete it, the application will crash when close because the compiler will ignore this lambda and cause empty implementation exception.
		                            });
	}
	
	std::cout << "press to stop clients" << std::endl;
	std::cin.get();
	
	for (size_t i = 0; i < NumberOfClient; i++)
	{
		clients[i]->Disconnect();
	}
	
	std::cout << server.GetSessionMap().size() << std::endl;
	std::cout << "press to end server" << std::endl;
	std::cin.get();
	
	std::cout << temp1 << " " << temp2 << " " << temp3 << " " << temp4 << std::endl;
	std::cout << "press to exit" << std::endl;
	std::cin.get();
}