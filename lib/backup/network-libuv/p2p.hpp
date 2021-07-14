#pragma once

#include <iostream>
#include <mutex>
#include <condition_variable>

#include "network-common/i_p2p_node.hpp"
#include "tcp_client.hpp"
#include "tcp_server.hpp"
#include "exception.hpp"

namespace network
{
	class p2p : public i_p2p_node
	{
	public:
		p2p() : _service_running(false)
		{
		
		}
		
		void send(const std::string &ip, uint16_t port, address_type type, const char *data, size_t size, send_callback callback) override
		{
			send(ip,port,type, reinterpret_cast<const uint8_t*>(data), size, callback);
		}
		
		void send(const std::string &ip, uint16_t port, address_type type, const uint8_t *data, size_t size, send_callback callback) override
		{
			tcp_client _client;
			network::SocketAddr::IPV ip_type;
			if (type == ipv4) ip_type = network::SocketAddr::Ipv4;
			else if (type == ipv6) ip_type = network::SocketAddr::Ipv6;
			else THROW_NOT_IMPLEMENTED;
			network::SocketAddr addr(ip, port, ip_type);
			bool connected = false, get_reply = false, send_success = false;
			std::string received_data;
			std::mutex m;
			std::condition_variable cv;
			
			_client.setConnectStatusCallback([&data, &size, &connected, &get_reply, &send_success, &cv, &_client](tcp_client::ConnectStatus status)
			                                 {
				                                 if (status == tcp_client::ConnectStatus::OnConnectSuccess)
				                                 {
					                                 connected = true;
					                                 _client.writeInLoop(reinterpret_cast<const char *>(data), size, [&get_reply, &send_success, &cv](WriteInfo &info)
					                                 {
						                                 if (info.status == 0)
						                                 {
							                                 //success
							                                 send_success = true;
						                                 }
						                                 else
						                                 {
							                                 //fail
							                                 cv.notify_one();
						                                 }
					                                 });
				                                 }
				                                 else // connect fail
				                                 {
				                                    cv.notify_one();
				                                 }
			                                 });
			_client.setMessageCallback([&get_reply, &cv, &received_data](const char *data, ssize_t size)
			                           {
				                           get_reply = true;
				                           received_data.assign(data, size);
				                           cv.notify_one();
			                           });
			_client.connect(addr);
			_client.start();
			
			//wait for reply or error
			{
				std::unique_lock<std::mutex> lk(m);
				cv.wait(lk);
			}
			
			_client.close();
			
			send_packet_status status = send_callback_not_specified;
			if (!connected) status = send_packet_connection_fail;
			if (!send_success) status = send_packet_write_fail;
			if (get_reply) status = send_packet_success;
			else status = send_packet_no_reply;
			if (callback != nullptr)
			{
				callback(status, received_data.data(), received_data.size());
			}
		}
		
		void start_service(uint16_t port) override
		{
			if (_service_running)
				return;
			network::SocketAddr addr("0.0.0.0", port, uv::SocketAddr::Ipv4);
			_server.bindAndListen(addr);
			_server.start();
			_service_running = true;
		}
		
		void stop_service() override
		{
			if (!_service_running)
				return;
			
			_server.close();
			_service_running = false;
		}
		
		void set_receive_callback(receive_callback callback) override
		{
			_callback = callback;
			_server.setMessageCallback([this](TcpConnectionPtr ptr, const char* data, ssize_t size){
				std::string reply = _callback(data, int(size));
				ptr->write(reply.data(), reply.size(), [&ptr](WriteInfo& info){
					if (info.status == 0)
					{
						//success
						ptr->close([](std::string&){});
					}
					else
					{
						//error to send the reply
						ptr->close([](std::string&){});
					}
				});
			});
		}
	
	private:
		tcp_server _server;
		std::mutex _client_mutex;
		bool _service_running;
		receive_callback _callback;
	};
}
