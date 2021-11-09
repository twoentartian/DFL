#pragma once

#include <mutex>
#include <condition_variable>

#include <glog/logging.h>
#include <boost/format.hpp>
#include "network-common/i_p2p_node.hpp"
#include "tcp_simple_server.hpp"
#include "tcp_simple_client.hpp"

namespace network
{
	class p2p : public i_p2p_node
	{
	public:
		p2p(bool enable_log = false) : _service_running(false), _enable_log(enable_log)
		{
		
		}
		
		void send(const std::string &ip, uint16_t port, address_type type, const char *data, size_t size, send_callback callback) override
		{
			send(ip, port, type, reinterpret_cast<const uint8_t *>(data), size, callback);
		}
		
		void send(const std::string &ip, uint16_t port, address_type type, const uint8_t *data, size_t size, send_callback callback) override
		{
			std::string received_data;
			std::mutex m;
			std::condition_variable cv;
			send_packet_status p2p_status = send_packet_not_specified;
			
			std::shared_ptr<simple::tcp_client> client = simple::tcp_client::CreateClient();
			client->connect(ip, port);
			client->SetConnectHandler([&ip, &port, &data, &size, &cv, &p2p_status, this](tcp_status status, std::shared_ptr<simple::tcp_client> client)
			                          {
				                          if (status == tcp_status::Success)
				                          {
					                          if (_enable_log)
						                          LOG(INFO) << boost::format("[p2p] connection to %1%:%2% built") % ip % port;
					                          p2p_status = send_packet_no_reply;
					                          tcp_status send_status;
					                          try
					                          {
						                          send_status = client->write(data, size);
					                          }
					                          catch (...)
					                          {
						                          LOG(WARNING) << boost::format("[p2p] failed to send request to %1%:%2%") % ip % port;
						                          return;
					                          }

					                          if (send_status != Success)
					                          {
						                          p2p_status = send_packet_connection_fail;
						                          cv.notify_one();
					                          }
				                          }
				                          else
				                          {
					                          if (_enable_log)
						                          LOG(INFO) << boost::format("[p2p] fail to connect to %1%:%2%") % ip % port;
					                          p2p_status = send_packet_connection_fail;
					                          cv.notify_one();
				                          }
			                          });
			
			client->SetReceiveHandler([&cv, &received_data, &p2p_status](char *data, uint32_t length, std::shared_ptr<simple::tcp_client> client)
			                          {
				                          received_data.assign(reinterpret_cast<char *>(data), length);
				                          p2p_status = send_packet_success;
				                          cv.notify_one();
			                          });
			client->SetCloseHandler([this](const std::string &ip, uint16_t port)
			                        {
				                        if (_enable_log)
					                        LOG(INFO) << boost::format("[p2p] connection to %1%:%2% close") % ip % port;
			                        });
			
			//wait for reply
			{
				std::unique_lock<std::mutex> lk(m);
				cv.wait_for(lk, std::chrono::seconds(5));
			}
			
			client->Disconnect();
			if (callback != nullptr)
			{
				callback(p2p_status, received_data.data(), received_data.size());
			}
		}
		
		void start_service(uint16_t port, int worker) override
		{
			_server.SetAcceptHandler([this](const std::string &ip, uint16_t port, std::shared_ptr<simple::tcp_session> session)
			                         {
				                         if (_enable_log)
					                         LOG(INFO) << "[p2p] server accept: " << ip << ":" << port;
			                         });
			_server.SetSessionReceiveHandler([this](char *data, uint32_t length, std::shared_ptr<simple::tcp_session> session_receive)
			                                 {
				                                 if (_enable_log)
					                                 LOG(INFO) << "[p2p] server receive packet with size: " << length;
				
				                                 //process the packet
				                                 if (_callback)
				                                 {
					                                 auto reply = _callback(data, length, session_receive->ip());
					                                 try
					                                 {
					                                 	
						                                 session_receive->write(reply.data(), reply.size());
					                                 }
					                                 catch (...)
					                                 {
						                                 LOG(WARNING) << boost::format("[p2p] failed to send reply to %1%:%2%") % session_receive->ip() % session_receive->port();
						                                 return;
					                                 }
				                                 }
			                                 });
			_server.SetSessionCloseHandler([&](std::shared_ptr<simple::tcp_session> session_close)
			                               {
				                               if (_enable_log)
					                               LOG(INFO) << "[p2p] server: session close";
			                               });
			
			auto ret_code = _server.Start(port, worker);
			CHECK_EQ(ret_code, Success) << "[p2p] error to start server at port: " << port << ", message: " << std::to_string(ret_code);
		}
		
		void start_service(uint16_t port) override
		{
			start_service(port, 2);
		}
		
		void stop_service() override
		{
			_server.Stop();
		}
		
		void set_receive_callback(receive_callback callback = nullptr) override
		{
			_callback = callback;
		}
		
		uint16_t read_port() const override
		{
			return _server.read_port();
		}
	
	private:
		const bool _enable_log;
		simple::tcp_server _server;
		std::mutex _client_mutex;
		bool _service_running;
		receive_callback _callback;
	};
	
}
