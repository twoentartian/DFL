#pragma once

#include <mutex>
#include <condition_variable>

#include <glog/logging.h>
#include <boost/format.hpp>
#include "network-common/i_p2p_node_with_header.hpp"
#include "tcp_simple_server_with_header.hpp"
#include "tcp_simple_client_with_header.hpp"

namespace network
{
	class p2p_with_header : i_p2p_node_with_header
	{
	public:
		static constexpr int DEFAULT_WAIT_TIME = 10;
		int no_response_wait_time;
		
		p2p_with_header(bool enable_log = false) : _enable_log(enable_log), no_response_wait_time(DEFAULT_WAIT_TIME)
		{
		
		}
		
		void send(const std::string &ip, uint16_t port, i_p2p_node_with_header::address_type type, header::COMMAND_TYPE command, const char *data, size_t size, i_p2p_node_with_header::send_callback callback) override
		{
			send(ip, port, type, command, reinterpret_cast<const uint8_t *>(data), size, callback);
		}
		
		void send(const std::string &ip, uint16_t port, i_p2p_node_with_header::address_type type, header::COMMAND_TYPE command, const uint8_t *data, size_t size, i_p2p_node_with_header::send_callback callback) override
		{
			std::string received_data;
			std::mutex m;
			std::condition_variable cv;
			i_p2p_node_with_header::send_packet_status p2p_status = i_p2p_node_with_header::send_packet_not_specified;
			std::shared_ptr<simple::tcp_client_with_header> client = simple::tcp_client_with_header::CreateClient();
			header::COMMAND_TYPE received_command;
			client->SetConnectHandler([&ip, &port, &data, &size, &cv, &command, &p2p_status, this](tcp_status status, std::shared_ptr<simple::tcp_client> client)
			                          {
				                          if (status == tcp_status::Success)
				                          {
					                          if (_enable_log)
						                          LOG(INFO) << boost::format("[p2p] connection to %1%:%2% built") % ip % port;
					                          p2p_status = i_p2p_node_with_header::send_packet_no_reply;
					                          tcp_status send_status;
					                          try
					                          {
						                          send_status = std::static_pointer_cast<simple::tcp_client_with_header>(client)->write_with_header(command, data, size);
					                          }
					                          catch (...)
					                          {
						                          LOG(WARNING) << boost::format("[p2p] failed to send request to %1%:%2%") % ip % port;
						                          return;
					                          }
					
					                          if (send_status != Success)
					                          {
						                          p2p_status = i_p2p_node_with_header::send_packet_connection_fail;
						                          cv.notify_one();
					                          }
				                          }
				                          else
				                          {
					                          if (_enable_log)
						                          LOG(INFO) << boost::format("[p2p] fail to connect to %1%:%2%") % ip % port;
					                          p2p_status = i_p2p_node_with_header::send_packet_connection_fail;
					                          cv.notify_one();
				                          }
			                          });
			client->SetReceiveHandler_with_header([&cv, &received_data, &p2p_status, &received_command](header::COMMAND_TYPE command, std::shared_ptr<std::string> data, std::shared_ptr<simple::tcp_client_with_header> client)
			                                      {
				                                      received_data.assign(*data);
				                                      p2p_status = i_p2p_node_with_header::send_packet_success;
				                                      cv.notify_one();
				                                      received_command = command;
			                                      });
			client->SetCloseHandler([this](const std::string &ip, uint16_t port)
			                        {
				                        if (_enable_log)
					                        LOG(INFO) << boost::format("[p2p] connection to %1%:%2% close") % ip % port;
			                        });
			client->connect(ip, port);
			//wait for reply
			{
				std::unique_lock<std::mutex> lk(m);
				cv.wait_for(lk, std::chrono::seconds(no_response_wait_time));
			}
			
			client->Disconnect();
			if (callback != nullptr)
			{
				callback(p2p_status, received_command, received_data.data(), received_data.size());
			}
		}
		
		void start_service(uint16_t port, int worker) override
		{
			_server.SetAcceptHandler([this](const std::string &ip, uint16_t port, std::shared_ptr<simple::tcp_session> session)
			                         {
				                         if (_enable_log)
					                         LOG(INFO) << "[p2p] server accept: " << ip << ":" << port;
			                         });
			_server.SetSessionReceiveHandler_with_header([this](header::COMMAND_TYPE command, std::shared_ptr<std::string> data, std::shared_ptr<simple::tcp_session_with_header> session_receive)
			                                             {
				                                             if (_enable_log)
					                                             LOG(INFO) << "[p2p] server receive packet with size: " << data->length();
				
				                                             //process the packet
				                                             if (_callback)
				                                             {
					                                             auto reply = _callback(command, data->data(), data->length(), session_receive->ip());
					                                             try
					                                             {
						                                             auto&[command, reply_str] = reply;
						                                             session_receive->write_with_header(command, reply_str.data(), reply_str.size());
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
		
		void set_receive_callback(receive_callback callback) override
		{
			_callback = callback;
		}
		
		uint16_t read_port() const override
		{
			return _server.read_port();
		}
		
	private:
		const bool _enable_log;
		simple::tcp_server_with_header _server;
		std::mutex _client_mutex;
		bool _service_running;
		receive_callback _callback;
	};
	
}
