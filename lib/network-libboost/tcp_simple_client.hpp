#pragma once

#include <memory>
#include <functional>

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind/bind.hpp>

#include "tcp_status.hpp"

namespace network::simple
{
	class tcp_client : public std::enable_shared_from_this<tcp_client>
	{
	public:
		using CloseHandlerType = std::function<void(const std::string &, uint16_t)>;
		using ReceiveHandlerType = std::function<void(char*, uint32_t, std::shared_ptr<tcp_client>)>;
		using ConnectHandlerType = std::function<void(tcp_status, std::shared_ptr<tcp_client>)>;
		static constexpr int DEFAULT_WORKER_COUNT = 2;
		static constexpr int BUFFER_SIZE = 1000 * 1000 * 10; // 10MB
		static constexpr uint32_t DEFAULT_MTU = 1000 * 1000 * 10;
		
		static std::shared_ptr<tcp_client> CreateClient()
		{
			std::shared_ptr<tcp_client> tempClientPtr(new tcp_client());
			return tempClientPtr;
		}
		
		~tcp_client() noexcept(false)
		{
			if (_connected)
			{
				//throw an exception here because any false use of tcp::tcp_client will cause this exception.
				throw std::logic_error("destruct the client while the client is in use"); //Call Disconnect before destruction.
			}
			delete[] _buffer;
			
			--_instanceCounter;
			if (_instanceCounter == 0)
			{
				//Last target destruct.
				_io_service->stop();
				for (auto &&thread : _client_thread_container)
				{
					thread.join();
				}
				_client_thread_container.clear();
			}
		}
		
		[[nodiscard]] std::string ip() const
		{
			return _ip;
		}
		
		[[nodiscard]] uint16_t port() const
		{
			return _port;
		}
		
		tcp_status connect(const std::string &ip, uint16_t port, bool wait_for_connect = false)
		{
			_ip = ip;
			_port = port;
			
			auto self(shared_from_this());
			
			boost::asio::ip::tcp::endpoint endPoint(boost::asio::ip::address::from_string(ip), port);
			
			_socket.reset(new boost::asio::ip::tcp::socket(*_io_service));
			_connecting = true;
			_socket->async_connect(endPoint, boost::bind(&tcp_client::connect_handler, this, boost::asio::placeholders::error, _socket));
			
			if (!wait_for_connect)
			{
				return Success;
			}
			else
			{
				while (_connecting)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
				if (_connected)
				{
					return Success;
				}
				else
				{
					return ConnectionFailed;
				}
			}
		}
		
		void Disconnect()
		{
			try
			{
				if (_socket)
				{
					_socket->cancel();
					_socket->shutdown(boost::asio::socket_base::shutdown_both);
					_socket->close();
				}
			}
			catch (const std::exception &e)
			{
			
			}
			
			WaitForClose();
		}
		
		tcp_status write(const char *data, uint32_t length)
		{
			return write(reinterpret_cast<const uint8_t *>(data), length);
		}
		
		tcp_status write(const uint8_t *data, uint32_t length)
		{
			uint32_t current_mtu = _mtu;
			uint32_t remain_length = length;
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
		
		[[nodiscard]] bool isConnected() const
		{
			return _connected;
		}
		
		[[nodiscard]] bool isConnecting() const
		{
			return _connecting;
		}
		
		void SetReceiveHandler(ReceiveHandlerType handler)
		{
			_receiveHandlers.push_back(handler);
		}
		
		void SetCloseHandler(CloseHandlerType handler)
		{
			_closeHandlers.push_back(handler);
		}
		
		void SetConnectHandler(ConnectHandlerType handler)
		{
			_connectHandlers.push_back(handler);
		}
		
		void WaitForClose() const
		{
			while (_connected || _connecting)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}
		
		void WaitForConnecting() const
		{
			while (_connecting)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}
	
	protected:
		static std::vector<std::thread> _client_thread_container;
		static std::shared_ptr<boost::asio::io_service> _io_service;
		static size_t _worker;
		static std::atomic<uint32_t> _instanceCounter;
		uint32_t _mtu;
		
		std::shared_ptr<boost::asio::ip::tcp::socket> _socket;
		std::vector<ReceiveHandlerType> _receiveHandlers;
		std::vector<CloseHandlerType> _closeHandlers;
		std::vector<ConnectHandlerType> _connectHandlers;
		std::atomic<bool> _connected;
		std::atomic<bool> _connecting;
		char* _buffer;
		std::string _ip;
		uint16_t _port;
		std::mutex _writer_locker;
		
		explicit tcp_client(bool lastTarget = false) : _connected(false), _connecting(false), _buffer(nullptr), _mtu(DEFAULT_MTU)
		{
			++_instanceCounter;
			if (lastTarget)
			{
				static boost::asio::io_service::work work(*_io_service);
				return;
			}
			
			if (!_io_service)
			{
				_io_service.reset(new boost::asio::io_service());
			}
			if (_client_thread_container.empty())
			{
				static tcp_client lastTargetInstance(true);
				_worker = DEFAULT_WORKER_COUNT;
				for (size_t i = 0; i < _worker; i++)
				{
					_client_thread_container.emplace_back([this]()
					                                      {
						                                      _io_service->run();
					                                      });
				}
			}
			
			_buffer = new char[BUFFER_SIZE];
		}
		
		void close()
		{
			if (_connected)
			{
				for (auto &&close_handler : _closeHandlers)
				{
					close_handler(_ip, _port);
				}
				_connected = false;
			}
		}
		
		void connect_handler(const boost::system::error_code &ec, std::shared_ptr<boost::asio::ip::tcp::socket> socket)
		{
			std::shared_ptr<tcp_client> self;
			_connecting = false;
			try
			{
				self = shared_from_this();
			}
			catch (...)
			{
				return;
			}
			
			for (auto &&single_connect_handler : _connectHandlers)
			{
				if (ec.failed())
				{
					single_connect_handler(ConnectionFailed, self);
				}
				else
				{
					single_connect_handler(Success, self);
				}
			}
			if (ec.failed())
			{
				_connected = false;
				return;
			}
			else
			{
				_connected = true;
			}
			
			//set socket option
			//_socket->set_option(boost::asio::socket_base::receive_buffer_size(BUFFER_SIZE));
			//_socket->set_option(boost::asio::socket_base::send_buffer_size(BUFFER_SIZE));
			//_socket->set_option(boost::asio::ip::tcp::no_delay(true));
			
			do_read();
		}
		
		virtual void do_read()
		{
			_socket->async_read_some(boost::asio::buffer(_buffer, BUFFER_SIZE), [this](const boost::system::error_code &ec, size_t received_length)
			{
				if (ec)
				{
					close();
					return;
				}
				auto self(shared_from_this());
				for (auto &&receive_handler : _receiveHandlers)
				{
					receive_handler(_buffer, received_length, self);
				}
				do_read();
			});
		}
	};
	std::vector<std::thread> tcp_client::_client_thread_container;
	std::shared_ptr<boost::asio::io_service> tcp_client::_io_service;
	size_t tcp_client::_worker;
	std::atomic<uint32_t> tcp_client::_instanceCounter;
	
}