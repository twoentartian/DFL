#pragma once

#include "tcp_protocol_impl.hpp"
#include "tcp_status.hpp"

namespace network
{
	typedef Protocol_StartSignWithLengthInfo_AsioSocket_Async PROTOCOL_ABS;
	
	class tcp_point
	{
	public:
		virtual tcp_status SendRaw(const uint8_t *data, uint32_t length) = 0;
		
		virtual tcp_status SendProtocol(const uint8_t *data, uint32_t length, uint32_t speedLimit = 0, uint32_t packet_size = 0) = 0;
		
		[[nodiscard]] virtual std::string ip() const = 0;
		
		[[nodiscard]] virtual uint16_t port() const = 0;
		
		[[nodiscard]] virtual bool isConnected() const = 0;
		
		[[nodiscard]] virtual bool isReceiving() const = 0;
		
		[[nodiscard]] virtual bool isSending() const = 0;
		
		virtual void Disconnect() const = 0;
		
		virtual void WaitForClose() const = 0;
	};
	
}

namespace std
{
	std::string to_string(network::tcp_status status)
	{
		switch (status)
		{
			case network::tcp_status::Success:
				return "success";
			case network::tcp_status::PortNotValid:
				return "port is not valid";
			case network::tcp_status::PortOccupied:
				return "port is occupied";
			case network::tcp_status::AddressNotValid:
				return "address is not valid";
			case network::tcp_status::InvalidArgs:
				return "args are not valid";
			case network::tcp_status::PotentialErrorPacket_LengthNotIdentical:
				return "the length of the packet does not identical to the named length";
			case network::tcp_status::SocketCorrupted:
				return "the socket might have been close";
			case network::tcp_status::ConnectionFailed:
				return "connect failed";
			case network::tcp_status::NoSenseOperation:
				return "this operation does not make any sense. This operation might have been done before and it cannot be executed twice";
			default:
				return "not defined";
		}
	}
}
