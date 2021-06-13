#pragma once

#include <functional>
#include <tuple>

namespace network
{
	class i_p2p_node
	{
	public:
		enum send_packet_status
		{
			send_callback_not_specified = 0,
			send_packet_success,
			send_packet_connection_fail,
			send_packet_write_fail,
			send_packet_no_reply,
			
			send_packet_status_size
		};
		
		enum address_type
		{
			not_specified = 0,
			ipv4,
			ipv6,
			
			address_type_size
		};
		
		using send_callback = std::function<void(send_packet_status, const char* data, int length)>;
		using receive_callback = std::function<std::string(const char* data, int length)>;
		
		virtual void send(const std::string &ip, uint16_t port, address_type type, const char* data, size_t size, send_callback callback) = 0;
		
		virtual void send(const std::string& ip, uint16_t port, address_type type, const uint8_t* data, size_t size, send_callback callback) = 0;
		
		virtual void start_service(uint16_t port) = 0;
		
		virtual void stop_service() = 0;
		
		virtual void set_receive_callback(receive_callback callback) = 0;
	};
}
