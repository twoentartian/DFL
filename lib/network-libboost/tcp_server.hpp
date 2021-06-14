#pragma once

#include <memory>
#include <functional>

#include <boost/system/error_code.hpp>
#include <boost/asio.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/bind/bind.hpp>

#include "tcp_common.hpp"
#include "tcp_protocol_impl.hpp"
#include "util.hpp"

namespace network
{
	class tcp_server;
	
	class tcp_session;
	
	class tcp_session : public std::enable_shared_from_this<tcp_session>, public tcp_point
	{
	public:
		using CloseHandlerType = std::function<void(std::shared_ptr<tcp_session>)>;
		using ReceiveHandlerType = std::function<void(uint8_t *, uint32_t, tcp_status, std::shared_ptr<tcp_session>)>;
		using ReceiveErrorHandlerType = std::function<void(ProtocolStatus, std::shared_ptr<tcp_session>)>;
		
		uint64_t id;
		
		[[nodiscard]] bool isConnected() const override
		{
			return _connected;
		}
		
		[[nodiscard]] bool isReceiving() const override
		{
			return _receiving;
		}
		
		[[nodiscard]] bool isSending() const override
		{
			return _sending;
		}
		
		[[nodiscard]] std::string ip() const override
		{
			return _ip;
		}
		
		[[nodiscard]] uint16_t port() const override
		{
			return _port;
		}
		
		tcp_session(const std::shared_ptr<boost::asio::ip::tcp::socket> &socket_ptr)
		{
			++_session_counter;
			
			_socket = socket_ptr;
			_connected = true;
			_receiving = false;
			const boost::asio::socket_base::receive_buffer_size option(1024 * 1024 * 1);
			_socket->set_option(option);
			
			_ip = _socket->remote_endpoint().address().to_string();
			_port = _socket->remote_endpoint().port();
			
			_meter = std::make_shared<flux_meter>();
			_meter->Start();
		}
		
		~tcp_session()
		{
			--_session_counter;
			
			Disconnect();
			_meter->StopMeter();
		}
		
		void SetCloseHandler(const CloseHandlerType &handler)
		{
			_closeHandlers.push_back(handler);
		}
		
		void SetReceiveHandler(const ReceiveHandlerType &handler)
		{
			_receiveHandlers.push_back(handler);
		}
		
		void SetReceiveErrorHandler(const ReceiveErrorHandlerType &handler)
		{
			_receiveErrorHandlers.push_back(handler);
		}
		
		tcp_status SendRaw(const uint8_t* data, uint32_t length) override
		{
			util::bool_setter<std::atomic<bool>> sendSetter(_sending);
			std::lock_guard<std::mutex> lock_guard(_writer_locker);
			try
			{
				_socket->write_some(boost::asio::buffer(data, length));
			}
			catch (const std::exception &)
			{
				return SocketCorrupted;
			}
			return Success;
		}
		
		tcp_status SendRaw(const char *data, uint32_t length)
		{
			return SendRaw(reinterpret_cast<const uint8_t *>(data), length);
		}
		
		tcp_status SendProtocol(const uint8_t* data, uint32_t length, uint32_t speedLimit = 0, uint32_t packet_size = 0) override
		{
			util::bool_setter<std::atomic<bool>> sendSetter(_sending);
			std::lock_guard<std::mutex> lock_guard(_writer_locker);
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
		
		void start()
		{
			do_read();
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
		
		void WaitForClose() const override
		{
			while (_connected)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}
	
	private:
		friend class tcp_server;
		static uint32_t _session_counter;
		
		std::atomic<bool> _connected;
		std::atomic<bool> _receiving;
		std::atomic<bool> _sending;
		std::string _ip;
		uint16_t _port;
		std::mutex _writer_locker;
		std::shared_ptr<flux_meter> _meter;
		
		std::shared_ptr<boost::asio::ip::tcp::socket> _socket;
		std::vector<CloseHandlerType> _closeHandlers;
		std::vector<ReceiveHandlerType> _receiveHandlers;
		std::vector<ReceiveErrorHandlerType> _receiveErrorHandlers;
		std::vector<CloseHandlerType>* _server_closeHandlers;
		std::vector<ReceiveHandlerType>* _server_receiveHandlers;
		std::vector<ReceiveErrorHandlerType>* _server_receiveErrorHandlers;
		uint8_t *_buffer;
		
		PROTOCOL_ABS _protocol;
		
		void close()
		{
			if (!_connected)
			{
				return;
			}
			
			auto self(shared_from_this());
			for (auto &&close_handler : *_server_closeHandlers)
			{
				close_handler(self);
			}
			for (auto &&close_handler : _closeHandlers)
			{
				close_handler(self);
			}
			
			_connected = false;
		}
		
		void do_read()
		{
			_protocol.Receive_GetSize(_socket, [&](const uint32_t est_length, ProtocolStatus status)
			{
				if (status == ProtocolStatus::SocketError)
				{
					close();
					return;
				}
				if (status != ProtocolStatus::Success)
				{
					do_read();
					for (auto &&receive_error_handler : *_server_receiveErrorHandlers)
					{
						auto self(shared_from_this());
						receive_error_handler(status, self);
					}
					for (auto &&receive_error_handler : _receiveErrorHandlers)
					{
						auto self(shared_from_this());
						receive_error_handler(status, self);
					}
					return;
				}
				
				if (est_length)
				{
					_receiving = true;
					_buffer = new uint8_t[est_length];
					//auto* buffer = new uint8_t[length];
					
					_protocol.Receive(_socket, _buffer, est_length, [&, est_length](uint32_t real_receive_length, ProtocolStatus receive_status)
					{
						if (receive_status == ProtocolStatus::SocketError)
						{
							delete[] _buffer;
							close();
							_receiving = false;
							return;
						}
						if (receive_status != ProtocolStatus::Success)
						{
							delete[] _buffer;
							do_read();
							_receiving = false;
							return;
						}
						
						//Check whether the receive handler is set.
						for (auto &&receive_handler : *_server_receiveHandlers)
						{
							auto self(shared_from_this());
							if (real_receive_length == est_length)
							{
								receive_handler(_buffer, est_length, Success, self);
							}
							else
							{
								receive_handler(_buffer, real_receive_length, PotentialErrorPacket_LengthNotIdentical, self);
							}
						}
						for (auto &&receive_handler : _receiveHandlers)
						{
							auto self(shared_from_this());
							if (real_receive_length == est_length)
							{
								receive_handler(_buffer, est_length, Success, self);
							}
							else
							{
								receive_handler(_buffer, real_receive_length, PotentialErrorPacket_LengthNotIdentical, self);
							}
						}

						
						delete[] _buffer;
						
						//Continue receive data
						_receiving = false;
						do_read();
					});
				}
				else
				{
					do_read();
				}
			});
		}
		
		friend class tcp_server;
		
		void closeByServer()
		{
			if (!_connected)
			{
				return;
			}
			_socket->shutdown(boost::asio::socket_base::shutdown_both);
			_socket->close();
			_meter->StopMeter();
			
			auto self(shared_from_this());
			
			//Ignore the first close handler set by server.
			for (size_t i = 1; i < _closeHandlers.size(); i++)
			{
				_closeHandlers[i](self);
			}
			for (auto &&close_handler : *_server_closeHandlers)
			{
				close_handler(self);
			}
			
			_connected = false;
		}
	};
	
	uint32_t tcp_session::_session_counter = 0;
	
	class tcp_server
	{
	public:
		using AcceptHandlerType = std::function<void(const std::string &, uint16_t, std::shared_ptr<tcp_session>)>;
		
		tcp_server(bool EnableSessionControl = false) : _sessionControlEnabled(EnableSessionControl)
		{
			_io_service.reset(new boost::asio::io_service());
		}
		
		~tcp_server()
		{
			Stop();
			
			//_io_service.reset();	//Never destruct the io_service because the destruction of acceptor will destruct the io_service.
			_acceptor.reset();
			_end_point.reset();
		}
		
		class TheClientsOfServer : std::enable_shared_from_this<TheClientsOfServer>
		{
		public:
			using SessionMapType = std::unordered_map<uint16_t, std::shared_ptr<tcp_session>>;
			
			SessionMapType SessionContainer;
			RWLock LockSessionContainer;
		
		private:
			
		};
		
		tcp_status Start(uint16_t port, uint32_t worker = 1)
		{
			if (worker < 1) worker = 1;
			if (worker > std::thread::hardware_concurrency()) worker = std::thread::hardware_concurrency();
			
			try
			{
				_end_point.reset(new boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));
				_acceptor.reset(new boost::asio::ip::tcp::acceptor(*_io_service, *_end_point, false));
			}
			catch (...)
			{
				return PortOccupied;
			}
			
			try
			{
				accept();
			}
			catch (...)
			{
				return PortOccupied;
			}
			
			_worker = worker;
			addWorkerForReceiveService();
			
			return Success;
		}
		
		void CloseAllSessions()
		{
			if (_sessionControlEnabled)
			{
				_lockClientMap_WithSessionControl.LockWrite();
				for (auto &&client : _clientMap_WithSessionControl)
				{
					client.second->LockSessionContainer.LockWrite();
					for (auto &&session : client.second->SessionContainer)
					{
						session.second->closeByServer();
					}
					client.second->LockSessionContainer.UnlockWrite();
				}
				_clientMap_WithSessionControl.clear();
				_lockClientMap_WithSessionControl.UnlockWrite();
			}
			else
			{
				_lockSessionMap_WithoutSessionControl.LockWrite();
				auto iterator = _sessionMap_WithoutSessionControl.begin();
				while (iterator != _sessionMap_WithoutSessionControl.end())
				{
					iterator->second->closeByServer();
					_sessionMap_WithoutSessionControl.erase(iterator++);
				}
				_lockSessionMap_WithoutSessionControl.UnlockWrite();
			}
		}
		
		void Stop()
		{
			//Stop Acceptor
			if (_acceptor)
			{
				_acceptor->close();
			}
			else
			{
				//The server in fact does not start yet
				return;
			}
			
			//Stop IO service
			if (!_io_service->stopped())
			{
				_io_service->stop();
			}
			
			//Wait for thread exit
			for (auto &&thread : _acceptor_thread_container)
			{
				thread.join();
			}
			
			//Clean _acceptor_thread_container
			_acceptor_thread_container.clear();
			
			//Close all sessions
			CloseAllSessions();
		}
		
		void SetAcceptHandler(const AcceptHandlerType &handler)
		{
			_acceptHandler.push_back(handler);
		}
		
		std::optional<std::shared_ptr<tcp_session>> GetSession(const std::string &ip, uint16_t port)
		{
			if (_sessionControlEnabled)
			{
				lock_guard_write <RWLock> lock_guard(_lockClientMap_WithSessionControl);
				const auto result_client = _clientMap_WithSessionControl.find(ip);
				auto &client = *(result_client->second);
				if (result_client != _clientMap_WithSessionControl.end())
				{
					lock_guard_write <RWLock> lock_guard_client(client.LockSessionContainer);
					const auto result_port = client.SessionContainer.find(port);
					if (result_port != client.SessionContainer.end())
					{
						return {result_port->second};
					}
					else
					{
						return {};
					}
				}
				else
				{
					return {};
				}
				return {};
			}
			else
			{
				return {};
			}
		}
		
		std::optional<std::shared_ptr<tcp_session>> GetSession(uint64_t session_id)
		{
			if (_sessionControlEnabled)
			{
				return {};
			}
			else
			{
				lock_guard_write <RWLock> lock_guard(_lockSessionMap_WithoutSessionControl);
				auto result = _sessionMap_WithoutSessionControl.find(session_id);
				if (result != _sessionMap_WithoutSessionControl.end())
				{
					return {result->second};
				}
				else
				{
					return {};
				}
			}
		}
		
		void broadcast(const std::string &message)
		{
			for (auto &&client : _clientMap_WithSessionControl)
			{
				for (auto &&port_session : client.second->SessionContainer)
				{
					auto&[port, session] = port_session;
					session->SendRaw(message.data(), message.size());
				}
			}
		}
		
		void SetCloseHandler(const tcp_session::CloseHandlerType &handler)
		{
			_closeHandlers.push_back(handler);
		}
		
		void SetReceiveHandler(const tcp_session::ReceiveHandlerType &handler)
		{
			_receiveHandlers.push_back(handler);
		}
		
		void SetReceiveErrorHandler(const tcp_session::ReceiveErrorHandlerType &handler)
		{
			_receiveErrorHandlers.push_back(handler);
		}
	
	private:
		RWLock _lock_acceptor_thread_container;
		std::vector<std::thread> _acceptor_thread_container;
		std::shared_ptr<boost::asio::io_service> _io_service;
		std::shared_ptr<boost::asio::ip::tcp::acceptor> _acceptor;
		std::shared_ptr<boost::asio::ip::tcp::endpoint> _end_point;
		std::vector<AcceptHandlerType> _acceptHandler;
		size_t _worker;
		
		void accept()
		{
			const auto socket_ptr = std::make_shared<boost::asio::ip::tcp::socket>(*_io_service);
			
			_acceptor->async_accept(*socket_ptr, boost::bind(&tcp_server::accept_handler, this, socket_ptr, boost::asio::placeholders::error));
		}
		
		bool remove_single_session(const std::string &ip, uint16_t port)
		{
			if (_sessionControlEnabled)
			{
				lock_guard_write <RWLock> lock_guard(_lockClientMap_WithSessionControl);
				const auto result_client = _clientMap_WithSessionControl.find(ip);
				auto &client = *(result_client->second);
				if (result_client != _clientMap_WithSessionControl.end())
				{
					lock_guard_write <RWLock> lock_guard_client(client.LockSessionContainer);
					const auto result_port = client.SessionContainer.find(port);
					if (result_port != client.SessionContainer.end())
					{
						client.SessionContainer.erase(result_port);
					}
					else
					{
						return false;
					}
				}
				else
				{
					return false;
				}
				
				//Delete client without any connection
				{
					lock_guard_read <RWLock> lock_guard_client(client.LockSessionContainer);
					if (client.SessionContainer.empty())
					{
						_clientMap_WithSessionControl.erase(result_client);
					}
				}
				
				return true;
			}
			else
			{
				return false;
			}
		}
		
		bool remove_single_session(uint64_t session_id)
		{
			if (_sessionControlEnabled)
			{
				return false;
			}
			else
			{
				lock_guard_write <RWLock> lock_guard(_lockSessionMap_WithoutSessionControl);
				auto result = _sessionMap_WithoutSessionControl.find(session_id);
				if (result != _sessionMap_WithoutSessionControl.end())
				{
					_sessionMap_WithoutSessionControl.erase(result);
					return true;
				}
				else
				{
					return false;
				}
			}
		}
		
		void accept_handler(const std::shared_ptr<boost::asio::ip::tcp::socket> &socket_ptr, const boost::system::error_code &ec)
		{
			accept();
			if (!ec)
			{
				std::shared_ptr<tcp_session> clientSession = std::make_shared<tcp_session>(socket_ptr);
				clientSession->_server_closeHandlers = &this->_closeHandlers;
				clientSession->_server_receiveErrorHandlers = &this->_receiveErrorHandlers;
				clientSession->_server_receiveHandlers = &this->_receiveHandlers;
				
				const auto remote_endpoint = socket_ptr->remote_endpoint();
				const std::string &remote_ip = remote_endpoint.address().to_string();
				const uint16_t &remote_port = remote_endpoint.port();
				
				//Session control
				if (_sessionControlEnabled)
				{
					lock_guard_write lgd_wsc(_lockClientMap_WithSessionControl);
					const auto clientMap_result = _clientMap_WithSessionControl.find(remote_ip);
					if (clientMap_result == _clientMap_WithSessionControl.end())
					{
						//Create new client
						auto tempClient = std::make_shared<TheClientsOfServer>();
						lock_guard_write lgd_tcw(tempClient->LockSessionContainer);
						tempClient->SessionContainer.insert(std::make_pair(remote_port, clientSession));        //We do not need to add index here because the index is only used in WithoutSessionControl situation.
						_clientMap_WithSessionControl.insert(std::make_pair(remote_ip, tempClient));
					}
					else
					{
						//Modify current clients
						auto &tempClient = clientMap_result->second;
						lock_guard_write lgd_tcw(tempClient->LockSessionContainer);
						const auto sessionMap_result = tempClient->SessionContainer.find(remote_port);
						if (sessionMap_result == tempClient->SessionContainer.end())
						{
							tempClient->SessionContainer.insert(std::make_pair(remote_port, clientSession));
						}
						else
						{
							throw std::logic_error("Impossible situation: a same endpoint connect to this server for twice, please consider logic error");
						}
					}
				}
				else
				{
					uint64_t index = getAvailableIndexInSessionMap_WithoutSessionControl();
					clientSession->id = index;
					lock_guard_write lgd_wsc(_lockSessionMap_WithoutSessionControl);
					_sessionMap_WithoutSessionControl.insert(std::make_pair(index, clientSession));
				}
				
				//Add worker
				addWorkerForReceiveService();
				
				//Set close handler
				clientSession->SetCloseHandler([&](const std::shared_ptr<tcp_session> &session)
				                               {
					                               //Session control
					                               if (_sessionControlEnabled)
					                               {
						                               lock_guard_write lgd_wsc(_lockClientMap_WithSessionControl);
						                               auto clientMap_result = _clientMap_WithSessionControl.find(session->ip());
						                               if (clientMap_result == _clientMap_WithSessionControl.end())
						                               {
							                               throw std::logic_error("Impossible situation: the client is not stored in this map, please consider logic error");
						                               }
						                               else
						                               {
							                               //Modify current clients
							                               auto &tempClient = clientMap_result->second;
							                               lock_guard_write lgd_wtc(tempClient->LockSessionContainer);
							                               const auto sessionMap_result = tempClient->SessionContainer.find(session->port());
							                               if (sessionMap_result == tempClient->SessionContainer.end())
							                               {
								                               throw std::logic_error("Impossible situation: the session is not stored in this map, please consider logic error");
							                               }
							                               else
							                               {
								                               tempClient->SessionContainer.erase(sessionMap_result);
							                               }
						                               }
						
						                               //Determine whether to delete the client in client map
						                               if (clientMap_result->second->SessionContainer.empty())
						                               {
							                               _clientMap_WithSessionControl.erase(clientMap_result);
						                               }
					                               }
					                               else
					                               {
						                               lock_guard_write lgd_wsc(_lockClientMap_WithSessionControl);
						                               const auto result = _sessionMap_WithoutSessionControl.find(session->id);
						                               if (result == _sessionMap_WithoutSessionControl.end())
						                               {
							                               throw std::logic_error("item should be here but it does not");
						                               }
						                               else
						                               {
							                               _sessionMap_WithoutSessionControl.erase(result);
						                               }
					                               }
				                               });
				
				//Accept Handler
				for (auto &&handler : _acceptHandler)
				{
					handler(remote_ip, remote_port, clientSession);
				}
				
				clientSession->start();
			}
		}
		
		void addWorkerForReceiveService()
		{
			const int32_t diff = size_t(tcp_session::_session_counter) + _worker - _acceptor_thread_container.size();
			if (diff > 0)
			{
				lock_guard_write <RWLock> lock_guard(_lock_acceptor_thread_container);
				for (size_t i = 0; i < diff; i++)
				{
					_acceptor_thread_container.emplace_back([&]()
					                                        {
						                                        _io_service->run();
					                                        });
				}
			}
		}
		
		//Session control region
	public:
		bool IsSessionControlEnabled() const
		{
			return _sessionControlEnabled;
		}
	
	private:
		friend class tcp_session;
		const bool _sessionControlEnabled;
		std::unordered_map<std::string, std::shared_ptr<TheClientsOfServer>> _clientMap_WithSessionControl;
		RWLock _lockClientMap_WithSessionControl;
		std::unordered_map<uint64_t, std::shared_ptr<tcp_session>> _sessionMap_WithoutSessionControl;
		RWLock _lockSessionMap_WithoutSessionControl;
		
		std::vector<tcp_session::CloseHandlerType> _closeHandlers;
		std::vector<tcp_session::ReceiveHandlerType> _receiveHandlers;
		std::vector<tcp_session::ReceiveErrorHandlerType> _receiveErrorHandlers;
		
		uint64_t getAvailableIndexInSessionMap_WithoutSessionControl()
		{
			static std::random_device rd;
			static std::default_random_engine random_engine(rd());
			static std::uniform_int_distribution<uint64_t> range(0, std::numeric_limits<uint64_t>::max());
			while (true)
			{
				uint64_t random = range(random_engine);
				
				_lockSessionMap_WithoutSessionControl.LockRead();
				const auto result = _sessionMap_WithoutSessionControl.find(random);
				if (result == _sessionMap_WithoutSessionControl.end())
				{
					_lockSessionMap_WithoutSessionControl.UnlockRead();
					return random;
				}
				else
				{
					_lockSessionMap_WithoutSessionControl.UnlockRead();
				}
			}
		}
	};
	
	
}