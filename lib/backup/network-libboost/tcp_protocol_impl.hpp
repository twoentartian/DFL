/**
 * This is the implemented protocol for the streamed asio packet constructing and parsing
 * The interface is defined in ./protocol.hpp, and three virtual functions should be implemented,
 * which are:
 *     void Send(boost::asio::ip::tcp::iostream& stream, uint8_t const* data, uint32_t length);
 *     uint32_t Receive_GetSize(boost::asio::ip::tcp::iostream& stream);
 *     void Receive(boost::asio::ip::tcp::iostream& stream, uint8_t* data, uint8_t length);
 */
#pragma once

#include "tcp_protocol.hpp"
#include "flux_meter.hpp"

/// The PACKAGE_START_FLAG is transmitted with little endian
/// the transmission process is equivalent to:
///    stream << 0x56 << 0xBC << 0xA5 << 0xF1
#define PACKAGE_START_FLAG    0xF1A5BC56

class Protocol_StartSignWithLengthInfo_AsioSocket_Async : Protocol_Async<std::shared_ptr<boost::asio::ip::tcp::socket>, flux_meter>
{
public:
	void Send(std::shared_ptr<boost::asio::ip::tcp::socket> &socket, uint8_t const *data, uint32_t length, std::shared_ptr<flux_meter> meter, uint32_t max_packet_size = 0) override
	{
		init_startSign();
		
		uint8_t start_lengthBuf[8];
		uint32_t length_copy = length;
		std::memcpy(start_lengthBuf, startSign, 4);
		for (int8_t temp = 3; temp >= 0; --temp)
		{
			start_lengthBuf[temp + 4] = static_cast<uint8_t>(length_copy & 0xff);
			length_copy >>= 8;
		}
		socket->write_some(boost::asio::buffer(start_lengthBuf, 8));
		
		if (max_packet_size == 0)
		{
			socket->write_some(boost::asio::buffer(data, length));
		} else
		{
			uint32_t pos = 0;
			while (pos < length)
			{
				const uint32_t remain = length - pos;
				const uint32_t send_amount = remain > max_packet_size ? max_packet_size : remain;
				socket->write_some(boost::asio::buffer(data + pos, send_amount));
				pos += send_amount;
				
				if (meter->IsUsed() && meter->IsRunning())
				{
					meter->PassFlux(send_amount);
					while (meter->IsLimitReached())
					{
						std::this_thread::sleep_for(std::chrono::milliseconds(1));
					}
				}
			}
		}

//Send stop sign
		uint8_t endBuf[4];
		for (int i = 0; i < 4; ++i)
		{
			endBuf[i] = startSign[3 - i];
		}
		socket->write_some(boost::asio::buffer(endBuf, 4));
	}
	
	void Send(std::shared_ptr<boost::asio::ip::tcp::socket> &socket, uint8_t const *data, uint32_t length) override
	{
		init_startSign();
		
		uint8_t start_lengthBuf[8];
		uint32_t length_copy = length;
		std::memcpy(start_lengthBuf, startSign, 4);
		for (int8_t temp = 3; temp >= 0; --temp)
		{
			start_lengthBuf[temp + 4] = static_cast<uint8_t>(length_copy & 0xff);
			length_copy >>= 8;
		}
		socket->write_some(boost::asio::buffer(start_lengthBuf, 8));
		char *dataChar = reinterpret_cast<char *>(const_cast<uint8_t *>(data));
		socket->write_some(boost::asio::buffer(dataChar, length));

//Send stop sign
		uint8_t endBuf[4];
		for (int i = 0; i < 4; ++i)
		{
			endBuf[i] = startSign[3 - i];
		}
		socket->write_some(boost::asio::buffer(endBuf, 4));
	}
	
	void Send_Async(std::shared_ptr<boost::asio::ip::tcp::socket> &socket, uint8_t const *data, uint32_t length, const std::function<void(uint32_t, ProtocolStatus)> &handler, std::shared_ptr<flux_meter> meter,
	                uint32_t max_packet_size = 0) override
	{
		init_startSign();
		
		uint32_t length_copy = length;

		//Send stop sign
		uint8_t endBuf[4];
		for (int i = 0; i < 4; ++i)
		{
			endBuf[i] = startSign[3 - i];
		}
		
		_do_packet_send_buffer = new uint8_t[length + 8 + 4];
		std::memcpy(_do_packet_send_buffer, startSign, 4);
		for (int8_t temp = 3; temp >= 0; --temp)
		{
			_do_packet_send_buffer[temp + 4] = static_cast<uint8_t>(length_copy & 0xff);
			length_copy >>= 8;
		}
		std::memcpy(_do_packet_send_buffer + 8, data, length);
		std::memcpy(_do_packet_send_buffer + 8 + length, endBuf, 4);
		
		_do_packet_send_handler = handler;
		_do_packet_send_length = length + 8 + 4;
		_do_packet_send_loc = 0;
		_do_packet_send_packet_size = max_packet_size;
		_do_packet_send_meter = meter;
		do_packet_send(socket, _do_packet_send_buffer, length + 8 + 4);
	}
	
	void Send_Async(std::shared_ptr<boost::asio::ip::tcp::socket> &socket, uint8_t const *data, uint32_t length, const std::function<void(uint32_t, ProtocolStatus)> &handler) override
	{
		init_startSign();
		
		uint32_t length_copy = length;

		//Send stop sign
		uint8_t endBuf[4];
		for (int i = 0; i < 4; ++i)
		{
			endBuf[i] = startSign[3 - i];
		}
		
		_do_packet_send_buffer = new uint8_t[length + 8 + 4];
		std::memcpy(_do_packet_send_buffer, startSign, 4);
		for (int8_t temp = 3; temp >= 0; --temp)
		{
			_do_packet_send_buffer[temp + 4] = static_cast<uint8_t>(length_copy & 0xff);
			length_copy >>= 8;
		}
		std::memcpy(_do_packet_send_buffer + 8, data, length);
		std::memcpy(_do_packet_send_buffer + 8 + length, endBuf, 4);
		
		_do_packet_send_handler = handler;
		_do_packet_send_length = length + 8 + 4;
		_do_packet_send_loc = 0;
		_do_packet_send_packet_size = 0;
		_do_packet_send_meter.reset();
		do_packet_send(socket, _do_packet_send_buffer, length + 8 + 4);
	}
	
	void Receive_GetSize(std::shared_ptr<boost::asio::ip::tcp::socket> &socket, const std::function<void(uint32_t length, ProtocolStatus status)> &handler) override
	{
		init_startSign();
		
		_begin_packet_receive_count = 0;
		_begin_packet_receive_handler = handler;
		begin_packet_receive(socket);
	}
	
	void Receive(std::shared_ptr<boost::asio::ip::tcp::socket> &socket, uint8_t *data, uint32_t length, const std::function<void(uint32_t real_receive_size, ProtocolStatus status)> &handler) override
	{
		_do_packet_receive_totalLength = length;
		_do_packet_receive_currentReadLoc = 0;
		_do_packet_receive_handler = handler;
		_do_packet_receive_data = data;
		
		do_packet_receive(socket);
	}

private:
	static uint8_t startSign[4];
	static uint8_t stopSign[4];
	static bool startSign_available;
	
	uint8_t *_do_packet_send_buffer;
	std::shared_ptr<flux_meter> _do_packet_send_meter;
	uint32_t _do_packet_send_loc;
	uint32_t _do_packet_send_packet_size;
	uint32_t _do_packet_send_length;
	std::function<void(uint32_t length, ProtocolStatus status)> _do_packet_send_handler;
	
	void do_packet_send(std::shared_ptr<boost::asio::ip::tcp::socket> &socket, uint8_t *data, uint32_t length)
	{
		socket->async_send(boost::asio::buffer(data, length), [&](const boost::system::error_code &ec, std::size_t size)
		{
			if (ec)
			{
				delete[] _do_packet_send_buffer;
				_do_packet_send_handler(_do_packet_send_loc, SocketError);
				return;
			}
			
			_do_packet_send_loc += size;
			if (_do_packet_send_loc == _do_packet_send_length)
			{
				delete[] _do_packet_send_buffer;
				_do_packet_send_handler(_do_packet_send_loc, Success);
				return;
			}
			
			//Meter flux limit
			if (_do_packet_send_meter && _do_packet_send_meter->IsUsed() && _do_packet_send_meter->IsRunning())
			{
				_do_packet_send_meter->PassFlux(size);
				while (_do_packet_send_meter->IsLimitReached())
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
			}
			
			//Send next data
			if (_do_packet_send_packet_size == 0)
			{
				do_packet_send(socket, _do_packet_send_buffer, _do_packet_send_length);
			} else
			{
				uint32_t next_packet_length = _do_packet_send_length - _do_packet_send_loc;
				next_packet_length = next_packet_length > _do_packet_send_packet_size ? _do_packet_send_packet_size : next_packet_length;
				do_packet_send(socket, _do_packet_send_buffer, next_packet_length);
			}
		});
	}
	
	uint8_t _begin_packet_receive_count;
	uint8_t _begin_packet_receive_buffer;
	char _begin_packet_receive_lengthBuf[4];
	std::function<void(uint32_t length, ProtocolStatus status)> _begin_packet_receive_handler;
	
	void begin_packet_receive(std::shared_ptr<boost::asio::ip::tcp::socket> &socket)
	{
		//socket->async_read_some(boost::asio::buffer(&_begin_packet_receive_buffer, 1), [&](const boost::system::error_code& ec, std::size_t received_length)
		socket->async_receive(boost::asio::buffer(&_begin_packet_receive_buffer, 1), [&](const boost::system::error_code &ec, std::size_t received_length)
		{
			if (ec)
			{
				_begin_packet_receive_handler(0, SocketError);
				return;
			}
			if (_begin_packet_receive_buffer == startSign[_begin_packet_receive_count])
			{
				_begin_packet_receive_count++;
				if (_begin_packet_receive_count == 4)
				{
					_begin_packet_receive_count = 0;
					socket->async_read_some(boost::asio::buffer(&_begin_packet_receive_lengthBuf, 4), [&](const boost::system::error_code &ec_l, size_t received_length_l)
					{
						if (received_length_l != 4)
						{
							_begin_packet_receive_handler(0, RemoteProtocolError);
						}
						if (ec_l)
						{
							_begin_packet_receive_handler(0, SocketError);
						}
						
						uint32_t est_length = 0;
						for (uint8_t temp = 0; temp < 4; ++temp)
						{
							est_length <<= 8;
							est_length = est_length | static_cast<uint32_t>(_begin_packet_receive_lengthBuf[temp] & 0x0000'00ff);
						}
						_begin_packet_receive_handler(est_length, Success);
					});
				} else
				{
					begin_packet_receive(socket);
				}
			} else
			{
				_begin_packet_receive_count = 0;
				begin_packet_receive(socket);
			}
		});
	}
	
	uint32_t _do_packet_receive_totalLength;
	uint32_t _do_packet_receive_currentReadLoc;
	uint8_t *_do_packet_receive_data;
	std::function<void(uint32_t real_receive_size, ProtocolStatus status)> _do_packet_receive_handler;
	
	void do_packet_receive(std::shared_ptr<boost::asio::ip::tcp::socket> &socket)
	{
		const uint32_t remain = _do_packet_receive_totalLength - _do_packet_receive_currentReadLoc;
		socket->async_read_some(boost::asio::buffer(_do_packet_receive_data + _do_packet_receive_currentReadLoc, remain), [&](const boost::system::error_code &ec, size_t received_length)
		{
			if (ec)
			{
				_do_packet_receive_handler(_do_packet_receive_currentReadLoc, SocketError);
				return;
			}
			
			_do_packet_receive_currentReadLoc += received_length;
			if (_do_packet_receive_currentReadLoc == _do_packet_receive_totalLength)
			{
				//Finish reading, time to receive stop sign
				uint8_t stopSignBuffer[4];
				socket->async_read_some(boost::asio::buffer(stopSignBuffer, 4), [&](const boost::system::error_code &ec_stopSign, size_t received_length_stopSign)
				{
					if (received_length_stopSign != 4)
					{
						_do_packet_receive_handler(_do_packet_receive_currentReadLoc, RemoteProtocolError);
					} else if (std::memcmp(stopSignBuffer, stopSign, 4) != 0)
					{
						_do_packet_receive_handler(_do_packet_receive_currentReadLoc, StopSignAbsence);
					} else
					{
						_do_packet_receive_handler(_do_packet_receive_currentReadLoc, Success);
					}
				});
			} else
			{
				//Continue reading
				do_packet_receive(socket);
			}
		});
	}
	
	static void init_startSign()
	{
		if (startSign_available)
		{
			return;
		}
		
		for (uint8_t count = 0; count < 4; ++count)
		{
			startSign[count] = static_cast<uint8_t>((PACKAGE_START_FLAG >> count * 8) & 0xff);
			stopSign[3 - count] = startSign[count];
		}
		startSign_available = true;
	}
};

uint8_t Protocol_StartSignWithLengthInfo_AsioSocket_Async::startSign[4];
uint8_t Protocol_StartSignWithLengthInfo_AsioSocket_Async::stopSign[4];
bool Protocol_StartSignWithLengthInfo_AsioSocket_Async::startSign_available = false;

class Protocol_StartSignWithLengthInfo_AsioSocket : Protocol_Sync<std::shared_ptr<boost::asio::ip::tcp::socket>, flux_meter>
{
public:
	void Send(std::shared_ptr<boost::asio::ip::tcp::socket> &socket, uint8_t const *data, uint32_t length, std::shared_ptr<flux_meter> meter, uint32_t max_packet_size = 1024 * 16) override
	{
		init_startSign();
		
		socket->write_some(boost::asio::buffer(startSign, 4));
		uint8_t lengthBuf[4];
		uint32_t length_copy = length;
		for (int8_t temp = 3; temp >= 0; --temp)
		{
			lengthBuf[temp] = static_cast<uint8_t>(length_copy & 0xff);
			length_copy >>= 8;
		}
		socket->write_some(boost::asio::buffer(lengthBuf, 4));
		
		if (max_packet_size == 0)
		{
			socket->write_some(boost::asio::buffer(data, length));
		} else
		{
			uint32_t pos = 0;
			while (pos < length)
			{
				const uint32_t remain = length - pos;
				const uint32_t send_amount = remain > max_packet_size ? max_packet_size : remain;
				socket->write_some(boost::asio::buffer(data + pos, send_amount));
				pos += send_amount;
				
				if (meter->IsUsed() && meter->IsRunning())
				{
					meter->PassFlux(send_amount);
					while (meter->IsLimitReached())
					{
						std::this_thread::sleep_for(std::chrono::milliseconds(1));
					}
				}
			}
		}
	}
	
	void Send(std::shared_ptr<boost::asio::ip::tcp::socket> &socket, uint8_t const *data, uint32_t length) override
	{
		init_startSign();
		
		socket->write_some(boost::asio::buffer(startSign, 4));
		uint8_t lengthBuf[4];
		uint32_t length_copy = length;
		for (int8_t temp = 3; temp >= 0; --temp)
		{
			lengthBuf[temp] = static_cast<uint8_t>(length_copy & 0xff);
			length_copy >>= 8;
		}
		socket->write_some(boost::asio::buffer(lengthBuf, 4));
		char *dataChar = reinterpret_cast<char *>(const_cast<uint8_t *>(data));
		socket->write_some(boost::asio::buffer(dataChar, length));
	}
	
	uint32_t Receive_GetSize(std::shared_ptr<boost::asio::ip::tcp::socket> &socket) override
	{
		init_startSign();
		
		uint8_t count = 0;
		uint8_t buffer;
		
		while (true)
		{
			socket->receive(boost::asio::buffer(&buffer, 1));
			if (buffer == startSign[count])
			{
				count++;
				if (count == 4)
				{
					count = 0;
					char lengthBuf[4];
					uint32_t length = 0;
					socket->receive(boost::asio::buffer(&lengthBuf, 4));
					for (uint8_t temp = 0; temp < 4; ++temp)
					{
						length <<= 8;
						length = length | static_cast<uint32_t>(lengthBuf[temp] & 0x0000'00ff);
					}
					return length;
				} else
				{
					continue;
				}
			} else
			{
				count = 0;
				continue;
			}
		}
		
		return 0;
	}
	
	uint32_t Receive(std::shared_ptr<boost::asio::ip::tcp::socket> &socket, uint8_t *data, uint32_t length) override
	{
		return socket->receive(boost::asio::buffer(data, length));
	}

private:
	static uint8_t startSign[4];
	static bool startSign_available;
	
	static void init_startSign()
	{
		if (startSign_available)
		{
			return;
		}
		
		for (uint8_t count = 0; count < 4; ++count)
		{
			startSign[count] = static_cast<uint8_t>((PACKAGE_START_FLAG >> count * 8) & 0xff);
		}
		startSign_available = true;
	}
};

uint8_t Protocol_StartSignWithLengthInfo_AsioSocket::startSign[4];
bool Protocol_StartSignWithLengthInfo_AsioSocket::startSign_available = false;

class Protocol_StartSignWithLengthInfo_AsioTcpStream : Protocol_Sync<boost::asio::ip::tcp::iostream, flux_meter>
{
public:
	void Send(boost::asio::ip::tcp::iostream &stream, uint8_t const *data, uint32_t length) override
	{
		init_startSign();
		
		if (stream)
		{
			/// Send start sign
			
			// The stream.read requires the type of inputted pointer to be char*
			// Here we use the reinterpret_cast operator to let the compiler ignore the mismatched type
			// (from cppreference: reinterpret_cast<> is purely a compile-time directive which instructs
			// the compiler to treat `expression` as if it has the type `new_type`)
			stream.write(reinterpret_cast<char *>(startSign), 4);
			
			/// Send length info
			uint8_t lengthBuf[4];
			uint32_t length_copy = length;
			for (int8_t temp = 3; temp >= 0; --temp)
			{
				lengthBuf[temp] = static_cast<uint8_t>(length_copy & 0xff);
				length_copy >>= 8;
			}
			//for (int8_t temp = 0; temp <= 3; ++temp)
			//{
			//	stream << lengthBuf[temp];
			//}
			stream.write(reinterpret_cast<char *>(lengthBuf), 4);
			
			/// Send data
			/**
			 * The first implementation causes an performance bottleneck
			 * by transmitting the data byte by byte through overloaded the stream insertion operator.
			 * While the native stream.write iterates the inputted buffer, which eliminates overhead
			 * operations and boost the performance
			 */
			// Implementation 1
			//for (uint32_t loc = 0; loc < length; ++loc)
			//{
			//	stream << data[loc];
			//}
			// Implementation 2
			char *dataChar = reinterpret_cast<char *>(const_cast<uint8_t *>(data));
			stream.write(dataChar, length);
		}
	}
	
	uint32_t Receive_GetSize(boost::asio::ip::tcp::iostream &stream) override
	{
		init_startSign();
		
		uint8_t count = 0;
		uint8_t buffer;
		while (stream)
		{
			stream.read(reinterpret_cast<char *>(&buffer), 1);
			if (buffer == startSign[count])
			{
				count++;
				if (count == 4)
				{
					count = 0;
					char lengthBuf[4];
					uint32_t length = 0;
					stream.read(lengthBuf, 4);
					for (uint8_t temp = 0; temp < 4; ++temp)
					{
						length <<= 8;
						length = length | static_cast<uint32_t>(lengthBuf[temp] & 0x0000'00ff);
					}
					return length;
				} else
				{
					continue;
				}
			} else
			{
				count = 0;
				continue;
			}
		}
		return 0;
	}
	
	uint32_t Receive(boost::asio::ip::tcp::iostream &stream, uint8_t *data, uint32_t length) override
	{
		//for (uint32_t i = 0; i < length; ++i)
		//{
		//	stream >> data[i];
		
		//}
		char *dataChar = reinterpret_cast<char *>(data);
		//char* buffer = new char[length];
		stream.read(dataChar, length);
		return 0;
		//for (size_t i = 0; i < length; ++i)
		//{
		//	data[i] = static_cast<uint8_t>(buffer[i]);
		//}
		//delete[] buffer;
	}

private:
	static uint8_t startSign[4];
	static bool startSign_available;
	
	static void init_startSign()
	{
		if (startSign_available)
		{
			return;
		}
		
		for (uint8_t count = 0; count < 4; ++count)
		{
			startSign[count] = static_cast<uint8_t>((PACKAGE_START_FLAG >> count * 8) & 0xff);
		}
		startSign_available = true;
	}
};

uint8_t Protocol_StartSignWithLengthInfo_AsioTcpStream::startSign[4];
bool Protocol_StartSignWithLengthInfo_AsioTcpStream::startSign_available = false;