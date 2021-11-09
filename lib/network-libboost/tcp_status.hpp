#pragma once

#include <boost/asio.hpp>

namespace network
{
	enum tcp_status
	{
		Success = 0,
		PortNotValid,
		PortOccupied,
		AddressNotValid,
		InvalidArgs,
		PotentialErrorPacket_LengthNotIdentical,
		SocketCorrupted,
		ConnectionFailed,
		NoSenseOperation,
		
		TcpStatus_Last_index
	};
	
	std::string local_address()
	{
		boost::asio::io_service ioService;
		boost::asio::ip::tcp::resolver resolver(ioService);
		return resolver.resolve(boost::asio::ip::host_name(), "")->endpoint().address().to_string();
	}
	
}