#pragma once

#include <memory>
#include <functional>

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind/bind.hpp>

#include "tcp_protocol.hpp"
#include "flux_meter.hpp"
#include "tcp_common.hpp"
#include <util.hpp>

namespace network
{
	class tcp_client : public std::enable_shared_from_this<tcp_client>, public tcp_point
	{
	public:
		using CloseHandlerType = std::function<void(const std::string &, uint16_t)>;
		using ReceiveHandlerType = std::function<void(uint8_t *, uint32_t, tcp_status, std::shared_ptr<tcp_client>)>;
		using ConnectHandlerType = std::function<void(tcp_status, std::shared_ptr<tcp_client>)>;
		using ReceiveErrorHandlerType = std::function<void(ProtocolStatus, std::shared_ptr<tcp_client>)>;
		
		static std::shared_ptr<tcp_client> CreateClient()
		{
			std::shared_ptr<tcp_client> tempClientPtr(new tcp_client());
			return tempClientPtr;
		}
		
		~tcp_client() noexcept(false)
		{
			if (_connected)
			{
				//throw an exception here because any invalid use of tcp::tcp_client will cause this exception.
				throw std::logic_error("destruct the client while the client is in use"); //Call Disconnect before destruction.
			}
			
			--_instanceCounter;
			if (_instanceCounter == 0)
			{
				//Last target destruct.
				_io_service->stop();
				for (auto &&thread : _client_thread_container)
				{
					thread.join();
				}
			}
			
			if (!_lastTarget)
			{
				//Normal target destruct.
				_meter->StopMeter();
			}
		}
		
		[[nodiscard]] std::string ip() const override
		{
			return _ip;
		}
		
		[[nodiscard]] uint16_t port() const override
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
		
		[[nodiscard]] bool isConnected() const override
		{
			return _connected;
		}
		
		[[nodiscard]] bool isConnecting() const
		{
			return _connecting;
		}
		
		[[nodiscard]] bool isSending() const override
		{
			return _sending;
		}
		
		[[nodiscard]] bool isReceiving() const override
		{
			return _receiving;
		}
		
		tcp_status SendRaw(const uint8_t* data, uint32_t length) override
		{
			std::lock_guard<std::mutex> lock_guard(_writer_locker);
			util::bool_setter<std::atomic<bool>> sendSetter(_sending);
			try
			{
				_socket->write_some(boost::asio::buffer(data, length));
			}
			catch (const std::exception &e)
			{
				return SocketCorrupted;
			}
			return Success;
		}
		
		tcp_status SendProtocol(const uint8_t* data, uint32_t length, uint32_t speedLimit = 0, uint32_t packet_size = 0) override
		{
			std::lock_guard<std::mutex> lock_guard(_writer_locker);
			util::bool_setter<std::atomic<bool>> sendSetter(_sending);
			try
			{
				_meter->SetLimit(speedLimit);
				_protocol.Send(_socket, data, length, _meter, packet_size);
			}
			catch (const std::exception &)
			{
				return SocketCorrupted;
			}
			return Success;
		}
		
		void Disconnect() const override
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
		
		void SetReceiveErrorHandler(ReceiveErrorHandlerType handler)
		{
			_receiveErrorHandlers.push_back(handler);
		}
		
		void WaitForClose() const override
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
	
	private:
		static std::shared_ptr<boost::asio::io_service> _io_service;
		static std::vector<std::thread> _client_thread_container;
		static RWLock _lock_client_thread_container;
		static bool _io_service_started;
		static size_t _worker;
		
		static tcp_client *_instance;
		static std::atomic<uint32_t> _instanceCounter;
		
		std::shared_ptr<boost::asio::ip::tcp::socket> _socket;
		std::vector<ReceiveHandlerType> _receiveHandlers;
		std::vector<CloseHandlerType> _closeHandlers;
		std::vector<ConnectHandlerType> _connectHandlers;
		std::vector<ReceiveErrorHandlerType> _receiveErrorHandlers;
		std::atomic<bool> _connected;
		std::atomic<bool> _connecting;
		std::atomic<bool> _sending;
		std::atomic<bool> _receiving;
		bool _lastTarget;
		uint8_t *_buffer;
		std::string _ip;
		uint16_t _port;
		std::mutex _writer_locker;
		std::shared_ptr<flux_meter> _meter;
		
		PROTOCOL_ABS _protocol;
		
		explicit tcp_client(bool lastTarget = false) : _connected(false), _connecting(false), _sending(false), _receiving(false), _lastTarget(lastTarget), _buffer(nullptr), _port(0), _protocol()
		{
			++_instanceCounter;
			if (lastTarget)
			{
				static boost::asio::io_service::work work(*_io_service);
				return;
			}
			
			if (_instance == nullptr)
			{
				if (!_io_service)
				{
					_io_service.reset(new boost::asio::io_service());
				}
				static tcp_client lastTargetInstance(true);
				_instance = &lastTargetInstance;
			}
			
			AddWorkerForReceiveService();
			
			_meter = std::make_shared<flux_meter>();
			_meter->Start();
		}
		
		static void AddWorkerForReceiveService()
		{
			const int32_t diff = _instanceCounter - 1 + _worker - _client_thread_container.size() / 2;
			if (diff > 0)
			{
				for (int32_t i = 0; i < diff * 2; i++)
				{
					_lock_client_thread_container.LockWrite();
					_client_thread_container.emplace_back([&]()
					                                      {
						                                      _io_service->run();
					                                      });
					_lock_client_thread_container.UnlockWrite();
				}
			}
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
			try
			{
				self = shared_from_this();
			}
			catch (...)
			{
				_connecting = false;
				return;
			}
			
			//Wait for a while to let TCP shake hands finish.
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			
			for (auto &&single_connect_handler : _connectHandlers)
			{
				if (ec.failed())
				{
					single_connect_handler(ConnectionFailed, self);
					_connecting = false;
				}
				else
				{
					_connected = true;
					single_connect_handler(Success, self);
				}
			}
			
			if (ec.failed())
			{
				return;
			}
			
			_connected = true;
			do_read();
			_connecting = false;
		}
		
		void do_read()
		{
			_protocol.Receive_GetSize(_socket, [&](uint32_t length, ProtocolStatus status)
			{
				if (status == ProtocolStatus::SocketError)
				{
					close();
					return;
				}
				if (status != ProtocolStatus::Success)
				{
					do_read();
					for (auto &&receive_error_handler : _receiveErrorHandlers)
					{
						auto self(shared_from_this());
						receive_error_handler(status, self);
					}
					return;
				}
				
				if (length)
				{
					_receiving = true;
					_buffer = new uint8_t[length];
					
					_protocol.Receive(_socket, _buffer, length, [&, length, this](uint32_t real_receive_length, ProtocolStatus receive_status)
					{
						if (receive_status == ProtocolStatus::SocketError)
						{
							delete[] _buffer;
							_receiving = false;
							close();
							return;
						}
						if (receive_status != ProtocolStatus::Success)
						{
							delete[] _buffer;
							_receiving = false;
							do_read();
							return;
						}
						
						//Check whether the receive handler is set.
						if (_receiveHandlers.empty())
						{
							//To tell developer that some messages will miss.
							std::cout << "WARNING: no receive handler" << std::endl;
						}
						
						auto self = shared_from_this();
						for (auto &&receive_handler : _receiveHandlers)
						{
							if (real_receive_length == length)
							{
								receive_handler(_buffer, length, Success, self);
								delete[] _buffer;
							}
							else
							{
								receive_handler(_buffer, real_receive_length, PotentialErrorPacket_LengthNotIdentical, self);
								delete[] _buffer;
							}
						}
						
						//Continue receive data
						_receiving = false;
						do_read();            //This part of code can be move before {//Check whether the receive handler is set.} to improve performance, however, it might cause memory exception.
					});
				}
			});
		}
	};
	
	std::vector<std::thread> tcp_client::_client_thread_container;
	RWLock tcp_client::_lock_client_thread_container;
	std::shared_ptr<boost::asio::io_service> tcp_client::_io_service;
	bool tcp_client::_io_service_started = false;
	size_t tcp_client::_worker;
	tcp_client *tcp_client::_instance = nullptr;
	std::atomic<uint32_t> tcp_client::_instanceCounter = 0;
}
