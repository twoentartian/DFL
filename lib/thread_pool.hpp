#pragma once

#include <chrono>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <tuple>

class thread_pool
{
public:
	using thread_delete_callback_type = std::function<void(int64_t)>;
	
	thread_pool() = default;
	
	//invoke this function will cause thread been replaced by an empty thread.
	int64_t insert_thread(std::function<void()> func)
	{
		int64_t microseconds_since_epoch;
		while (true)
		{
			auto time_point_now = std::chrono::high_resolution_clock::now();
			auto duration_since_epoch = time_point_now.time_since_epoch();
			microseconds_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(duration_since_epoch).count();
			{
				std::lock_guard guard(_lock);
				auto iter = _thread_pool.find(microseconds_since_epoch);
				if (iter == _thread_pool.end())
				{
					auto [iter_insert, status] = _thread_pool.emplace(microseconds_since_epoch, std::make_pair(std::thread([func, this, microseconds_since_epoch](){
						func();
						auto& [thread, status] = _thread_pool[microseconds_since_epoch];
						status = false;
					}), true));
					break;
				}
			}
		}
		return microseconds_since_epoch;
	}
	
	void auto_clean()
	{
		std::lock_guard guard(_lock);
		std::vector<int64_t> remove_list;
		for(auto&& iter : _thread_pool)
		{
			if (!iter.second.second)
			{
				iter.second.first.join();
				for (auto&& callback: _delete_callbacks)
				{
					callback(iter.first);
				}
				remove_list.push_back(iter.first);
			}
		}
		for (auto&& remove_item: remove_list)
		{
			_thread_pool.erase(remove_item);
		}
	}
	
	void wait_for_close()
	{
		std::lock_guard guard(_lock);
		std::vector<int64_t> remove_list;
		for(auto&& iter : _thread_pool)
		{
			iter.second.first.join();
			for (auto&& callback: _delete_callbacks)
			{
				callback(iter.first);
			}
		}
		for (auto&& remove_item: remove_list)
		{
			_thread_pool.erase(remove_item);
		}
		
	}
	
	void set_delete_callback(thread_delete_callback_type callback)
	{
		_delete_callbacks.push_back(callback);
	}
	
private:
	std::unordered_map<int64_t, std::pair<std::thread, bool>> _thread_pool;
	std::mutex _lock;
	std::vector<thread_delete_callback_type> _delete_callbacks;
};
