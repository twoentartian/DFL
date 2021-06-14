#define NETWORK_USE_LIBBOOST

#include <atomic>
#include <boost/format.hpp>
#include <boost/shared_array.hpp>
#include "network.hpp"


constexpr uint32_t dataLength = 10 * 1024;
std::atomic_int temp1 = 0, temp2 = 0, temp3 = 0, temp4 = 0;

int main()
{
	using namespace network;
	
	size_t global_counter = 2;
	while (global_counter--)
	{
		// Server
		std::recursive_mutex cout_lock;
		
		tcp_server server(true);
		std::atomic_int server_count = 0;
		
		server.SetAcceptHandler([&cout_lock, &server_count](const std::string &ip, uint16_t port, std::shared_ptr<tcp_session> session)
		                        {
			                        std::lock_guard<std::recursive_mutex> temp_lock_guard(cout_lock);
			                        std::cout << "[server] accept: " << ip << ":" << port << std::endl;
			                        temp1++;
		                        });
		server.SetReceiveHandler([&cout_lock, &server_count](uint8_t *data, uint32_t length, network::tcp_status status, std::shared_ptr<network::tcp_session> session_receive)
		                         {
			                         temp2++;
			                         if (status == tcp_status::Success)
			                         {
				                         std::lock_guard<std::recursive_mutex> temp_lock_guard(cout_lock);
				                         std::cout << "[server] receive (ok) length " << length << " count: " << server_count << std::endl;
				                         server_count++;
				                         temp3++;
				                         session_receive->SendProtocol(data, length);
			                         }
			                         else
			                         {
				                         std::lock_guard<std::recursive_mutex> temp_lock_guard(cout_lock);
				                         std::cout << "[server] receive (protocol error) length " << length << std::endl;
			                         }
		                         });
		server.SetReceiveErrorHandler([&](ProtocolStatus status, std::shared_ptr<tcp_session> session)
		                              {
			                              std::lock_guard<std::recursive_mutex> temp_lock_guard(cout_lock);
			                              std::cout << "[server] receive (fatal error)" << status << std::endl;
		                              });
		server.SetCloseHandler([&](std::shared_ptr<tcp_session> session_close)
		                       {
			                       std::lock_guard<std::recursive_mutex> temp_lock_guard(cout_lock);
			                       std::cout << "[server] close: " << session_close->ip() << ":" << session_close->port() << std::endl;
		                       });
		
		auto ret_code = server.Start(9000 + global_counter, 6);
		std::cout << "server: " << std::to_string(ret_code) << std::endl;
		
		std::cout << "press to start clients" << std::endl;
		std::cin.get();
		
		// tcp_client
		const int NumberOfClient = 10;
		std::shared_ptr<tcp_client> clients[NumberOfClient];
		int client_counts[NumberOfClient];
		
		for (size_t i = 0; i < NumberOfClient; i++)
		{
			clients[i] = tcp_client::CreateClient();
			client_counts[i] = 0;
		}
		
		for (size_t i = 0; i < NumberOfClient; i++)
		{
			clients[i]->connect("192.168.0.128", 9000 + global_counter);
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
					                              std::this_thread::sleep_for(std::chrono::milliseconds(500));
					                              auto status = client->SendProtocol(data, dataLength, 0, dataLength);
					                              if (status == network::Success)
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
						                              client->SendProtocol(data, dataLength, 0, dataLength);
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

//		while (true)
//		{
//			bool exit = true;
//			for (size_t i = 0; i < NumberOfClient; i++)
//			{
//				if (client_counts[i] < 5)
//				{
//					exit = false;
//					break;
//				}
//			}
//			if (exit)
//			{
//				break;
//			}
//		}
		
		for (size_t i = 0; i < NumberOfClient; i++)
		{
			clients[i]->Disconnect();
		}
		
		std::cout << "press to end server" << std::endl;
		std::cin.get();
		
		server.Stop();
		
		std::cout << temp1 << " " << temp2 << " " << temp3 << std::endl;
		std::cout << "press to exit" << std::endl;
		std::cin.get();
	}
}