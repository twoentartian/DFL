#pragma once

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
}