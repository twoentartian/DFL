#pragma once

#include <memory>

#include "tcp_simple_client.hpp"
#include "../network-common/packet_header.hpp"

namespace network::simple
{
	class tcp_client_with_header : public tcp_client
	{
	public:
		using ReceiveHandlerType_with_header = std::function<void(header::COMMAND_TYPE command, std::shared_ptr<std::string>, std::shared_ptr<tcp_client_with_header>)>;
		
		static std::shared_ptr<tcp_client_with_header> CreateClient()
		{
			std::shared_ptr<tcp_client_with_header> tempClientPtr(new tcp_client_with_header());
			return tempClientPtr;
		}
		
		void SetReceiveHandler_with_header(ReceiveHandlerType_with_header handler)
		{
			_receiveHandlers_with_header.push_back(handler);
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
		header_decoder _header_decoder;
		std::vector<ReceiveHandlerType_with_header> _receiveHandlers_with_header;
		
		tcp_client_with_header()
		{
			_header_decoder.set_receive_callback([this](uint16_t command, std::shared_ptr<std::string> data){
				auto self(shared_from_this());
				for (auto &&receive_handler : _receiveHandlers_with_header)
				{
					receive_handler(command, data, std::static_pointer_cast<tcp_client_with_header>(self));
				}
			});
			this->SetReceiveHandler([this](char* data, uint32_t length, std::shared_ptr<tcp_client> self){
				_header_decoder.receive_data(data, length);
			});
		}
		
		
	};
}