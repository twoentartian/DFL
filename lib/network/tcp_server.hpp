#pragma once

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <uv/include/uv11.hpp>
#include "uv_types.hpp"

namespace network
{
	class tcp_server
	{
	public:
		explicit tcp_server(bool tcpNoDelay = true)
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
		
		void start(size_t worker = 1)
		{
			if (_running_thread_count != 0)
			{
				return;
			}
			
			_thread_pool.reserve(worker);
			for (int i = 0; i < worker; ++i)
			{
				_thread_pool.emplace_back([this]()
				                          {
					                          _running_thread_count++;
					                          this->_loop->run();
					                          _running_thread_count--;
				                          });
			}
			
		}
		
		void close(const DefaultCallback& callback = nullptr)
		{
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
			for (auto&& single_thread : _thread_pool)
			{
				single_thread.join();
			}
			
			// erase loop in thread pool
			_thread_pool.clear();
			
			//waitForClose();

			_thread_pool.clear();
		}
		
		TcpConnectionPtr getConnection(const std::string& name)
		{
			_server->getConnection(name);
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
		
		//never try to close the server in any callback/handler
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
			//std::unique_lock<std::mutex> lk(_mutex);
			while (_running_thread_count != 0)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				//_state_change_cv.wait(lk);
			}
		}
	
	public:
		static void SetBufferMode(BufferMode mode)
		{
			uv::TcpServer::SetBufferMode(mode);
		}
	
	private:
		std::shared_ptr<uv::TcpServer> _server;
		std::shared_ptr<uv::EventLoop> _loop;
		std::vector<std::thread> _thread_pool;
		std::atomic_int _running_thread_count;
//		std::condition_variable _state_change_cv;
//		std::mutex _mutex;
	};
	
}