#pragma once

#include <functional>
#include <tuple>
#include <exception.hpp>

namespace network
{
	class i_p2p_node
	{
	public:
		enum send_packet_status
		{
			send_packet_not_specified = 0,
			send_packet_success,
			send_packet_connection_fail,
			send_packet_write_fail,
			send_packet_no_reply,
			
			send_packet_status_size
		};
		
		static constexpr char const* send_packet_status_message[send_packet_status_size] = {
				"not specified",
				"success",
				"connection fail",
				"write fail",
				"no reply"
		};
		
		enum address_type
		{
			not_specified = 0,
			ipv4,
			ipv6,
			
			address_type_size
		};
		
		using send_callback = std::function<void(send_packet_status, const char* data, int length)>;
		using receive_callback = std::function<std::string(const char* data, size_t length, std::string ip)>;
		
		virtual void send(const std::string &ip, uint16_t port, address_type type, const char* data, size_t size, send_callback callback) = 0;
		
		virtual void send(const std::string& ip, uint16_t port, address_type type, const uint8_t* data, size_t size, send_callback callback) = 0;
		
		virtual void start_service(uint16_t port) = 0;
		
		virtual void start_service(uint16_t port, int worker)
		{
			THROW_NOT_IMPLEMENTED;
		}
		
		virtual void stop_service() = 0;
		
		virtual void set_receive_callback(receive_callback callback) = 0;
		
		virtual uint16_t read_port() const = 0;
	};
}
