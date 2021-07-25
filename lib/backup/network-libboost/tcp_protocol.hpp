#pragma once

#include <tuple>
#include <iostream>
#include <memory>
#include <functional>
#include <boost/asio.hpp>

/**
 * @brief The protocol class is an abstraction of the data format used in TCP tcp_server and tcp_client data exchange.
 */
enum ProtocolStatus
{
	Success = 0,
	SocketError,
	RemoteProtocolError,
	StopSignAbsence,
	
	ProtocolStatus_LastIndex
};

template <typename CONNECTION_ABS, typename METER_TYPE>
class Protocol_Sync
{
public:
	/**
	 * @brief insert data into stream
	 */
	virtual void Send(CONNECTION_ABS& stream, uint8_t const* data, uint32_t length, std::shared_ptr<METER_TYPE> meter, uint32_t max_packet_size) = 0;
	
	/**
	 * @brief insert data into stream
	 */
	virtual void Send(CONNECTION_ABS& stream, uint8_t const* data, uint32_t length) = 0;
	
	/**
	 * @brief Data receiving, Step 1: fetch the number of byte transmitted(intended) from stream
	 */
	virtual uint32_t Receive_GetSize(CONNECTION_ABS& stream) = 0;
	
	/**
	 * @brief Data receiving, Step 2: receive the actual data from the stream
	 */
	virtual uint32_t Receive(CONNECTION_ABS& stream, uint8_t* data, uint32_t length) = 0;
};

/**
 * @brief The protocol class is an abstraction of the data format used in TCP tcp_server and tcp_client data exchange.
 */
template <typename CONNECTION_ABS, typename METER_TYPE>
class Protocol_Async
{
public:
	/**
	 * @brief insert data into stream
	 */
	virtual void Send(CONNECTION_ABS& stream, uint8_t const* data, uint32_t length, std::shared_ptr<METER_TYPE> meter, uint32_t max_packet_size) = 0;
	
	/**
	 * @brief insert data into stream
	 */
	virtual void Send(CONNECTION_ABS& stream, uint8_t const* data, uint32_t length) = 0;
	
	/**
	 * @brief insert data into stream
	 */
	virtual void Send_Async(CONNECTION_ABS& stream, uint8_t const* data, uint32_t length, const std::function<void(uint32_t, ProtocolStatus)>&, std::shared_ptr<METER_TYPE> meter, uint32_t max_packet_size) = 0;
	
	/**
	 * @brief insert data into stream
	 */
	virtual void Send_Async(CONNECTION_ABS& stream, uint8_t const* data, uint32_t length, const std::function<void(uint32_t, ProtocolStatus)>&) = 0;
	
	/**
	 * @brief Data receiving, Step 1: fetch the number of byte transmitted(intended) from stream
	 */
	virtual void Receive_GetSize(CONNECTION_ABS& stream, const std::function<void(uint32_t length, ProtocolStatus)>&) = 0;
	
	/**
	 * @brief Data receiving, Step 2: receive the actual data from the stream
	 */
	virtual void Receive(CONNECTION_ABS& stream, uint8_t* data, uint32_t length, const std::function<void(uint32_t real_receive_size, ProtocolStatus)>&) = 0;
};

namespace std
{
	std::string to_string(ProtocolStatus status)
	{
		switch (status)
		{
			case Success:
				return "success";
			case SocketError:
				return "socket might have been close";
			case RemoteProtocolError:
				return "remote server send a wrong format";
			case StopSignAbsence:
				return "remote server does not send stop sign";
			default:
				return "not defined";
		}
	}
}