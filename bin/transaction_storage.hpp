#pragma once

#include <memory>
#include <vector>
#include <string>
#include <mutex>
#include <functional>
#include <thread>

#include <transaction.hpp>

class transaction_storage
{
public:
	using event_callback_type = std::function<void(std::shared_ptr<std::vector<transaction>>)>;
	
	transaction_storage()
	{
		_container.reset(new std::vector<transaction>);
	}
	
	void set_event_trigger(size_t trigger)
	{
		_trigger = trigger;
		
		try_trigger();
	}
	
	void add_event_callback(event_callback_type callback)
	{
		_event_callbacks.push_back(callback);
	}
	
	void add(const transaction& trans)
	{
		std::lock_guard guard(_container_lock);
		_container->push_back(trans);
		
		try_trigger();
	}
	
	std::shared_ptr<std::vector<transaction>> get_transaction()
	{
		std::lock_guard guard(_container_lock);
		auto current_container = _container;
		_container.reset(new std::vector<transaction>);
		return current_container;
	}
	
private:
	std::shared_ptr<std::vector<transaction>> _container;
	std::mutex _container_lock;
	size_t _trigger;
	std::vector<event_callback_type> _event_callbacks;
	
	void try_trigger()
	{
		if (_container->size() >= _trigger)
		{
			std::thread temp_thread([this](){
				auto transactions = get_transaction();
				for (int i = 0; i < _event_callbacks.size(); ++i)
				{
					_event_callbacks[i](transactions);
				}
			});
			temp_thread.detach();
		}
	}
	
};
