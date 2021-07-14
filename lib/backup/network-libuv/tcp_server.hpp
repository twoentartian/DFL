#pragma once

#include <atomic>
#include <chrono>
#include <vector>
#include <map>
#include <string>
#include <uv/include/uv11.hpp>
#include "uv_types.hpp"

namespace network
{
	class tcp_server
	{
	public:
		using OnConnectionStatusCallback = uv::OnConnectionStatusCallback;
		using OnMessageCallback = uv::OnMessageCallback;
		
		using OnCloseCallback = uv::OnCloseCallback;
		using CloseCompleteCallback = uv::OnCloseCallback;
		
	public:
		explicit tcp_server(bool tcpNoDelay = true) : _running_thread_count(0)
		{
			_loop.reset(new uv::EventLoop);
			_server.reset(new uv::TcpServer(_loop.get(), tcpNoDelay));
		}
		
		~tcp_server()
		{
			close();
		}
		
		int bindAndListen(SocketAddr& addr)
		{
			return _server->bindAndListen(addr);
		}
		
		void start()
		{
			if (isRunning())
				return;
			
			_thread.reset(new std::thread([this](){
				_running_thread_count++;
				this->_loop->run();
				_running_thread_count--;
			}));
		}
		
		void close(const DefaultCallback& callback = nullptr)
		{
			if (isClosed())
				return;
			
			// close server
			if (callback)
			{
				_server->close(callback);
			}
			else
			{
				_server->close([](){});
			}
			
			// stop loop
			_loop->runInThisLoop([this]()
			{
				this->_loop->stop();
			});
			
			// wait for threads exit
			// if resource deadlock avoided thrown in join(),
			// check close() should not be invoked in tcp_server callback.
			_thread->join();
			
			// erase loop in thread pool
			_thread.reset();
		}
		
		TcpConnectionPtr getConnection(const std::string& name)
		{
			return _server->getConnection(name);
		}
		
		void closeConnection(const std::string& name)
		{
			_server->closeConnection(name);
		}
		
		void setNewConnectCallback(OnConnectionStatusCallback callback)
		{
			_server->setNewConnectCallback(callback);
		}
		
		void setConnectCloseCallback(OnConnectionStatusCallback callback)
		{
			_server->setConnectCloseCallback(callback);
		}
		
		void setMessageCallback(OnMessageCallback callback)
		{
			_server->setMessageCallback(callback);
		}
		
		void write(TcpConnectionPtr connection,const char* buf,unsigned int size, AfterWriteCallback callback = nullptr)
		{
			_server->write(connection, buf, size, callback);
		}
		
		void write(std::string& name,const char* buf,unsigned int size, AfterWriteCallback callback = nullptr)
		{
			_server->write(name, buf, size, callback);
		}
		
		void writeInLoop(TcpConnectionPtr connection,const char* buf,unsigned int size,AfterWriteCallback callback)
		{
			_server->writeInLoop(connection, buf, size, callback);
		}
		
		void writeInLoop(std::string& name,const char* buf,unsigned int size,AfterWriteCallback callback)
		{
			_server->writeInLoop(name, buf, size, callback);
		}
		
		void setTimeout(unsigned int seconds)
		{
			_server->setTimeout(seconds);
		}
		
		std::shared_ptr<EventLoop> get_loop()
		{
			return _loop;
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
		
		std::map<std::string, TcpConnectionPtr> getAllConnection()
		{
			return _server->getAllConnection();
		}
	
	public:
		static void SetBufferMode(BufferMode mode)
		{
			uv::TcpServer::SetBufferMode(mode);
		}
	
	private:
		std::shared_ptr<uv::TcpServer> _server;
		std::shared_ptr<uv::EventLoop> _loop;
		std::shared_ptr<std::thread> _thread;
		std::atomic_int _running_thread_count;
	};
	
}