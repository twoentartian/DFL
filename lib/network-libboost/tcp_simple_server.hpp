#pragma once

#include <memory>
#include <functional>
#include <unordered_map>

#include <boost/system/error_code.hpp>
#include <boost/asio.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/bind/bind.hpp>

#include "lock.hpp"
#include "tcp_status.hpp"

//simple tcp server is designed for short TCp connection, without built-in protocol.
namespace network::simple
{
	class tcp_session : public std::enable_shared_from_this<tcp_session>
	{
	public:
		static constexpr int BUFFER_SIZE = 1000 * 1000 * 10; // 10MB
		static constexpr uint32_t DEFAULT_MTU = 1000 * 1000 * 10;
		
		using CloseHandlerType = std::function<void(std::shared_ptr<tcp_session>)>;
		using ReceiveHandlerType = std::function<void(char *, uint32_t, std::shared_ptr<tcp_session>)>;
		
		tcp_session(const std::shared_ptr<boost::asio::ip::tcp::socket> &socket_ptr) noexcept : _mtu(DEFAULT_MTU)
		{
			_buffer = new char[BUFFER_SIZE];
			_connected = true;
			_socket = socket_ptr;
			//_socket->set_option(boost::asio::socket_base::receive_buffer_size(BUFFER_SIZE));
			//_socket->set_option(boost::asio::socket_base::send_buffer_size(BUFFER_SIZE));
			//_socket->set_option(boost::asio::ip::tcp::no_delay(true));
			
			_ip = _socket->remote_endpoint().address().to_string();
			_port = _socket->remote_endpoint().port();
		}
		
		~tcp_session()
		{
			Disconnect();
			delete[] _buffer;
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
		
		void start()
		{
			do_read();
		}
		
		void Disconnect() const
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
		
		void WaitForClose() const
		{
			while (_connected)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}
		
		void SetCloseHandler(const CloseHandlerType &handler)
		{
			_closeHandlers.push_back(handler);
		}
		
		void SetReceiveHandler(const ReceiveHandlerType &handler)
		{
			_receiveHandlers.push_back(handler);
		}
		
		[[nodiscard]] std::string ip() const
		{
			return _ip;
		}
		
		[[nodiscard]] uint16_t port() const
		{
			return _port;
		}
	
	private:
		friend class tcp_server;
	
	protected:
		std::string _ip;
		uint16_t _port;
		std::shared_ptr<boost::asio::ip::tcp::socket> _socket;
		bool _connected;
		char *_buffer;
		uint32_t _mtu;
		
		std::vector<CloseHandlerType> _closeHandlers;
		std::vector<ReceiveHandlerType> _receiveHandlers;
		std::vector<CloseHandlerType>* _server_closeHandlers;
		std::vector<ReceiveHandlerType>* _server_receiveHandlers;
		
		void do_read()
		{
			_socket->async_read_some(boost::asio::buffer(_buffer, BUFFER_SIZE), [&](const boost::system::error_code &ec, size_t received_length)
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
				for (auto &&receive_handler : *_server_receiveHandlers)
				{
					receive_handler(_buffer, received_length, self);
				}

				do_read();
			});
		}
		
		void close()
		{
			if (!_connected)
			{
				return;
			}
			try
			{
				_socket->shutdown(boost::asio::socket_base::shutdown_both);
			}
			catch (...)
			{
			
			}
			_socket->close();
			
			auto self(shared_from_this());
			for (auto &&close_handler : _closeHandlers)
			{
				close_handler(self);
			}
			for (auto &&close_handler : *_server_closeHandlers)
			{
				close_handler(self);
			}
			_connected = false;
		}
		
		void closeByServer()
		{
			if (!_connected)
			{
				return;
			}
			_socket->shutdown(boost::asio::socket_base::shutdown_both);
			_socket->close();
			
			//Ignore the first close handler set by server.
			auto self(shared_from_this());
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
	
	class tcp_server
	{
	public:
		using AcceptHandlerType = std::function<void(const std::string &, uint16_t, std::shared_ptr<tcp_session>)>;
		
		tcp_server()
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
		
		tcp_status Start(uint16_t port, uint32_t worker = 1)
		{
			if (!_acceptor_thread_container.empty()) return NoSenseOperation;
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
			for (size_t i = 0; i < _worker; i++)
			{
				_acceptor_thread_container.emplace_back([this]()
				                                        {
					                                        _io_service->run();
				                                        });
			}
			
			return Success;
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
		
		void CloseAllSessions()
		{
			lock_guard_write guard(_lockSessionMap);
			auto iterator = _sessionMap.begin();
			while (iterator != _sessionMap.end())
			{
				iterator->second->closeByServer();
				_sessionMap.erase(iterator++);
			}
		}
		
		void SetAcceptHandler(const AcceptHandlerType &handler)
		{
			_acceptHandler.push_back(handler);
		}
		
		void SetSessionCloseHandler(const tcp_session::CloseHandlerType &handler)
		{
			_closeHandlers.push_back(handler);
		}
		
		void SetSessionReceiveHandler(const tcp_session::ReceiveHandlerType &handler)
		{
			_receiveHandlers.push_back(handler);
		}
		
		const std::unordered_map<std::string, std::shared_ptr<tcp_session>>& GetSessionMap() const
		{
			return _sessionMap;
		}
	
		uint16_t read_port() const
		{
			return _end_point->port();
		}
		
	protected:
		std::vector<std::thread> _acceptor_thread_container;
		std::shared_ptr<boost::asio::io_service> _io_service;
		std::shared_ptr<boost::asio::ip::tcp::acceptor> _acceptor;
		std::shared_ptr<boost::asio::ip::tcp::endpoint> _end_point;
		std::vector<AcceptHandlerType> _acceptHandler;
		std::vector<tcp_session::CloseHandlerType> _closeHandlers;
		std::vector<tcp_session::ReceiveHandlerType> _receiveHandlers;
		size_t _worker;
		
		std::unordered_map<std::string, std::shared_ptr<tcp_session>> _sessionMap;
		RWLock _lockSessionMap;
		
		void accept()
		{
			const auto socket_ptr = std::make_shared<boost::asio::ip::tcp::socket>(*_io_service);
			
			_acceptor->async_accept(*socket_ptr, boost::bind(&tcp_server::accept_handler, this, socket_ptr, boost::asio::placeholders::error));
		}
		
		virtual void accept_handler(const std::shared_ptr<boost::asio::ip::tcp::socket> &socket_ptr, const boost::system::error_code &ec)
		{
			accept();
			if (!ec)
			{
				std::shared_ptr<tcp_session> clientSession = std::make_shared<tcp_session>(socket_ptr);
				clientSession->_server_closeHandlers = &this->_closeHandlers;
				clientSession->_server_receiveHandlers = &this->_receiveHandlers;
				
				const auto remote_endpoint = socket_ptr->remote_endpoint();
				const std::string remote_ip = remote_endpoint.address().to_string();
				const uint16_t remote_port = remote_endpoint.port();
				
				//Session control
				{
					lock_guard_write lgd_wsc(_lockSessionMap);
					_sessionMap[remote_ip + ":" + std::to_string(remote_port)] = clientSession;
				}
				
				//Set close handler
				clientSession->SetCloseHandler([this](const std::shared_ptr<tcp_session> &session)
				                               {
					                               //Session control
					                               lock_guard_write lgd_wsc(_lockSessionMap);
					                               const auto result = _sessionMap.find(session->ip() + ":" + std::to_string(session->port()));
					                               if (result == _sessionMap.end())
					                               {
						                               throw std::logic_error("item should be here but it does not");
					                               }
					                               else
					                               {
						                               _sessionMap.erase(result);
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
		
		
	};
	
	
}