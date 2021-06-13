#pragma once

#include <atomic>
#include <chrono>
#include <atomic>

#include <uv/include/uv11.hpp>
#include "uv_types.hpp"

namespace network
{
	class tcp_client
	{
	public:
		using ConnectStatus = uv::TcpClient::ConnectStatus;
		using ConnectStatusCallback = uv::TcpClient::ConnectStatusCallback;
		using NewMessageCallback = uv::NewMessageCallback;
		
	public:
		tcp_client(bool tcpNoDelay = true) : _use_external_loop(false), _running_thread_count(0)
		{
			_loop.reset(new uv::EventLoop);
			_client.reset(new uv::TcpClient(tcp_client::_loop.get(), tcpNoDelay));
		}
		
		tcp_client(EventLoop* loop, bool tcpNoDelay) : _use_external_loop(true), _running_thread_count(0)
		{
			_client.reset(new uv::TcpClient(loop, tcpNoDelay));
		}
		
		virtual ~tcp_client()
		{
			close();
		}
		
		bool isTcpNoDelay()
		{
			return _client->isTcpNoDelay();
		}
		
		void setTcpNoDelay(bool isNoDelay)
		{
			_client->setTcpNoDelay(isNoDelay);
		}
		
		void connect(SocketAddr& addr)
		{
			_client->connect(addr);
		}
		
		void start()
		{
			if (isRunning()) return;
			
			if (!_thread)
			{
				_thread.reset(new std::thread([this](){
					_running_thread_count++;
					this->_loop->run();
					_running_thread_count--;
				}));
			}
		}
		
		void close(std::function<void(uv::TcpClient*)> callback = nullptr)
		{
			if (isClosed()) return;
			
			if (callback)
			{
				_client->close([&callback](uv::TcpClient* client){
					callback(client);
				});
			}
			else
			{
				_client->close([](uv::TcpClient* client){
				
				});
			}
			
			// stop loop
			_loop->runInThisLoop([this]()
			                     {
				                     this->_loop->stop();
			                     });

			// wait for threads exit
			// if resource deadlock avoided thrown in join(),
			// check close() should not be invoked in tcp_client callback.
			_thread->join();

			// erase loop in thread pool
			_thread.reset();
		}
		
		void write(const char* buf, unsigned int size, AfterWriteCallback callback = nullptr)
		{
			_client->write(buf, size, callback);
		}
		
		void writeInLoop(const char* buf, unsigned int size, AfterWriteCallback callback)
		{
			_client->writeInLoop(buf, size, callback);
		}
		
		void setConnectStatusCallback(ConnectStatusCallback callback)
		{
			_client->setConnectStatusCallback(callback);
		}
		
		void setMessageCallback(NewMessageCallback callback)
		{
			_client->setMessageCallback(callback);
		}
		
		std::shared_ptr<uv::EventLoop> get_loop()
		{
			return _loop;
		}
		
		PacketBufferPtr getCurrentBuf()
		{
			return _client->getCurrentBuf();
		}
		
		[[nodiscard]] bool isRunning() const
		{
			return _running_thread_count != 0;
		}

		[[nodiscard]] bool isClosed() const
		{
			return _running_thread_count == 0;
		}
		
		void waitForClose()
		{
			while (_running_thread_count != 0)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}
	
	private:
		std::shared_ptr<uv::EventLoop> _loop;
		std::shared_ptr<std::thread> _thread;
		
		const bool _use_external_loop;
		std::shared_ptr<uv::TcpClient> _client;

		std::atomic_int _running_thread_count;
	};
	
	using TcpClientPtr = std::shared_ptr<tcp_client>;
}
