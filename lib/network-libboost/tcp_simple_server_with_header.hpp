#pragma once

#include <memory>

#include "tcp_simple_server.hpp"
#include "../network-common/packet_header.hpp"

namespace network::simple
{
	class tcp_session_with_header : public tcp_session
	{
	public:
		using ReceiveHandlerType_with_header = std::function<void(header::COMMAND_TYPE, std::shared_ptr<std::string>, std::shared_ptr<tcp_session_with_header>)>;
		
		tcp_session_with_header(const std::shared_ptr<boost::asio::ip::tcp::socket> &socket_ptr) : tcp_session(socket_ptr)
		{
			_header_decoder.set_receive_callback([this](header::COMMAND_TYPE command, std::shared_ptr<std::string> data){
				auto self(shared_from_this());
				for (auto &&receive_handler : _receiveHandlers_with_header)
				{
					receive_handler(command, data, std::static_pointer_cast<tcp_session_with_header>(self));
				}
				for (auto &&receive_handler : *_server_receiveHandlers_with_header)
				{
					receive_handler(command, data, std::static_pointer_cast<tcp_session_with_header>(self));
				}
			});
			
			SetReceiveHandler([this](char* data, uint32_t length, std::shared_ptr<tcp_session> session){
				_header_decoder.receive_data(data, length);
			});
		}
		
		tcp_status write_with_header(header::COMMAND_TYPE command, const uint8_t *data, uint32_t length)
		{
			packet_header _header(command, length);
			std::string header_str = _header.get_header_byte();
			
			uint32_t current_mtu = _mtu;
			uint32_t remain_length = header_str.size();
			try
			{
				while(remain_length > 0)
				{
					uint32_t current_length = remain_length > current_mtu ? current_mtu : remain_length;
					current_length = _socket->write_some(boost::asio::buffer(header_str.data() + (header_str.size() - remain_length), current_length));
					remain_length -= current_length;
				}
				//boost::asio::write(*_socket, boost::asio::buffer(header_str));
				//_socket->send(boost::asio::buffer(header_str));
			}
			catch (const std::exception &)
			{
				return SocketCorrupted;
			}
			
			remain_length = length;
			try
			{
				while (remain_length > 0)
				{
					uint32_t current_length = remain_length > current_mtu ? current_mtu : remain_length;
					//boost::asio::write(*_socket, boost::asio::buffer(data + (length - remain_length), current_length));
					//_socket->send(boost::asio::buffer(data + (length - remain_length), current_length));
					current_length = _socket->write_some(boost::asio::buffer(data + (length - remain_length), current_length));
					remain_length -= current_length;
				}
			}
			catch (const std::exception &)
			{
				return SocketCorrupted;
			}
			
			return Success;
		}
		
		tcp_status write_with_header(header::COMMAND_TYPE command, const char *data, uint32_t length)
		{
			return write_with_header(command, reinterpret_cast<const uint8_t *>(data), length);
		}
	
	private:
		friend class tcp_server_with_header;
	
	protected:
		std::vector<ReceiveHandlerType_with_header> _receiveHandlers_with_header;
		std::vector<ReceiveHandlerType_with_header>* _server_receiveHandlers_with_header;
		header_decoder _header_decoder;
	};
	
	class tcp_server_with_header : public tcp_server
	{
	public:
		void SetSessionReceiveHandler_with_header(const tcp_session_with_header::ReceiveHandlerType_with_header &handler)
		{
			_receiveHandlers_with_header.push_back(handler);
		}
	
	protected:
		std::vector<tcp_session_with_header::ReceiveHandlerType_with_header> _receiveHandlers_with_header;
		
		virtual void accept_handler(const std::shared_ptr<boost::asio::ip::tcp::socket> &socket_ptr, const boost::system::error_code &ec) override
		{
			accept();
			if (!ec)
			{
				std::shared_ptr<tcp_session_with_header> clientSession = std::make_shared<tcp_session_with_header>(socket_ptr);
				clientSession->_server_closeHandlers = &this->_closeHandlers;
				clientSession->_server_receiveHandlers = &this->_receiveHandlers;
				clientSession->_server_receiveHandlers_with_header = &this->_receiveHandlers_with_header;
				
				const auto remote_endpoint = socket_ptr->remote_endpoint();
				const std::string remote_ip = remote_endpoint.address().to_string();
				const uint16_t remote_port = remote_endpoint.port();
				
				//Session control
				{
					lock_guard_write lgd_wsc(_lockSessionMap);
					_sessionMap[remote_ip + ":" + std::to_string(remote_port)] = clientSession;
				}
				
				//Set close handler
				clientSession->SetCloseHandler([this](const std::shared_ptr<tcp_session> &session)
				                               {
					                               //Session control
					                               lock_guard_write lgd_wsc(_lockSessionMap);
					                               const auto result = _sessionMap.find(session->ip() + ":" + std::to_string(session->port()));
					                               if (result == _sessionMap.end())
					                               {
						                               throw std::logic_error("item should be here but it does not");
					                               }
					                               else
					                               {
						                               _sessionMap.erase(result);
					                               }
				                               });
				
				//Accept Handler
				for (auto &&handler : _acceptHandler)
				{
					handler(remote_ip, remote_port, clientSession);
				}
				
				clientSession->start();
			}
		}
	
	};
	
}