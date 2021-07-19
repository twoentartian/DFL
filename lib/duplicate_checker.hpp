#pragma once

#include <chrono>
#include <memory>
#include <thread>
#include <mutex>
#include <unordered_map>

template <typename T>
class duplicate_checker
{
public:
	duplicate_checker(int expire_time_second, int remove_expires_thread_interval = 60) : _expire_time_second(expire_time_second), _remove_expires_thread_interval(remove_expires_thread_interval), _running(true)
	{
		_remove_expires_thread.reset(new std::thread(&duplicate_checker::_remove_expires_thread_fun, this));
	}
	
	~duplicate_checker()
	{
		_running = false;
		_remove_expires_thread->join();
	}
	
	void add(const T& target)
	{
		uint64_t microseconds_since_epoch = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		{
			std::lock_guard guard(_lock);
			_data[target] = microseconds_since_epoch;
		}
	}
	
	bool find(const T& target)
	{
		std::lock_guard guard(_lock);
		auto iter = _data.find(target);
		if (iter == _data.end())
			return false;
		else
			return true;
	}
	
private:
	void _remove_expires_thread_fun()
	{
		while (_running)
		{
			uint64_t seconds_since_epoch = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			{
				std::lock_guard guard(_lock);
				std::vector<T> expire_list;
				for (auto& [key,value] : _data)
				{
					if (seconds_since_epoch - value >= _expire_time_second)
					{
						expire_list.push_back(key);
					}
				}
				for (auto& expire_item: expire_list)
				{
					_data.erase(expire_item);
				}
			}
			for (int i = 0; i < _remove_expires_thread_interval && _running; ++i)
			{
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
		}
	}
	
	bool _running;

	std::unordered_map<T, uint64_t> _data;
	std::mutex _lock;
	std::shared_ptr<std::thread> _remove_expires_thread;
	int _remove_expires_thread_interval;
	int _expire_time_second;
};