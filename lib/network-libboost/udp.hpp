#pragma once

#include <vector>
#include <functional>
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>

namespace network::udp
{
	template<uint32_t buffer_size = 1024 * 16>
	class udp_socket : boost::noncopyable
	{
	public:
		enum UdpSocketStatus
		{
			Success,
			Fail_CreateUdpSocketFail,
			Fail_SocketNotOpen,
			Fail_EmptyPort,
			UdpSocketStatus_LastIndex
		};
		
		using ReceiveHandler = std::function<void(std::shared_ptr<boost::asio::ip::udp::endpoint>, uint8_t *, std::size_t)>;
		using ReceiveErrorHandler = std::function<void(boost::system::error_code ec)>;
		
		udp_socket() = default;
		
		udp_socket(uint16_t port)
		{
			Open(port);
		}
		
		UdpSocketStatus Open()
		{
			if (!_remote_ep)
			{
				return Fail_EmptyPort;
			}
			try
			{
				_io_context.reset(new boost::asio::io_context);
				_socket.reset(new boost::asio::ip::udp::socket(*_io_context, *_remote_ep));
			}
			catch (...)
			{
				return Fail_CreateUdpSocketFail;
			}
			
			do_receive();
			
			_receiveThread.reset(new std::thread([this]()
			                                     {
				                                     _io_context->run();
			                                     }));
			return Success;
		}
		
		UdpSocketStatus Open(uint16_t port)
		{
			_remote_ep.reset(new boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port));
			return Open();
		}
		
		~udp_socket()
		{
			if (_io_context)
			{
				_io_context->stop();
			}
			if (_receiveThread)
			{
				_receiveThread->join();
			}
			
			_socket.reset();
			_io_context.reset();
			delete[] _receiveBuffer;
		}
		
		std::tuple<UdpSocketStatus, std::size_t> Send(const boost::asio::ip::udp::endpoint &remote_ep, const uint8_t *data, uint32_t length)
		{
			if (!is_open())
			{
				return {Fail_SocketNotOpen, 0};
			}
			const std::size_t transmit_size = _socket->send_to(boost::asio::buffer(data, length), remote_ep);
			return {Success, transmit_size};
		}
		
		std::tuple<UdpSocketStatus, std::size_t> Send(const boost::asio::ip::udp::endpoint &remote_ep, const std::string &data)
		{
			return Send(remote_ep, reinterpret_cast<const uint8_t *>(data.data()), data.length());
		}
		
		std::tuple<UdpSocketStatus, std::size_t> Send(const std::string &ip, uint16_t port, uint8_t *data, uint32_t length)
		{
			const boost::asio::ip::udp::endpoint remote_ep(boost::asio::ip::address_v4::from_string(ip), port);
			return Send(remote_ep, data, length);
		}
		
		std::tuple<UdpSocketStatus, std::size_t> Send(const std::string &ip, uint16_t port, const std::string &data)
		{
			const boost::asio::ip::udp::endpoint remote_ep(boost::asio::ip::address_v4::from_string(ip), port);
			return Send(remote_ep, data);
		}
		
		void SetReceiveHandler(const ReceiveHandler &handler)
		{
			_receiveHandlers.push_back(handler);
		}
		
		void SetReceiveErrorHandler(const ReceiveErrorHandler &handler)
		{
			_receiveErrorHandlers.push_back(handler);
		}
		
		std::shared_ptr<boost::asio::ip::udp::socket> GetSocket() const
		{
			return _socket;
		}
		
		bool is_open() const
		{
			if (!_socket)
			{
				return false;
			}
			return _socket->is_open();
		}
	
	private:
		std::shared_ptr<boost::asio::io_context> _io_context;
		std::shared_ptr<boost::asio::ip::udp::socket> _socket;
		uint8_t *_receiveBuffer = new uint8_t[buffer_size];
		std::shared_ptr<boost::asio::ip::udp::endpoint> _remote_ep;
		std::shared_ptr<std::thread> _receiveThread;
		std::vector<ReceiveHandler> _receiveHandlers;
		std::vector<ReceiveErrorHandler> _receiveErrorHandlers;
		
		void do_receive()
		{
			_socket->async_receive_from(boost::asio::buffer(_receiveBuffer, buffer_size), *_remote_ep, [this](boost::system::error_code ec, std::size_t bytes_received)
			{
				if (!ec && bytes_received > 0)
				{
					for (auto &&handler : _receiveHandlers)
					{
						handler(_remote_ep, _receiveBuffer, bytes_received);
					}
					do_receive();
				}
				else
				{
					for (auto &&handler : _receiveErrorHandlers)
					{
						handler(ec);
					}
					do_receive();
				}
			});
		}
		
	};
	
}