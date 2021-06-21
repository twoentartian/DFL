#pragma once

#include <memory>
#include <thread>
#include <functional>
#include <ml_layer/data_convert.hpp>

template<typename DType>
class data_generator
{
public:
	using generate_callback_type = std::function<void(std::tuple<std::vector<Ml::tensor_blob_like<DType>>, std::vector<Ml::tensor_blob_like<DType>>>)>;
	
	data_generator() : _running(true), _callback(nullptr), _interval(std::chrono::milliseconds(0)), _generate_size(1)
	{
	
	}
	
	void load_dataset(const std::string &data_filename, const std::string &label_filename)
	{
		_dataset.load_dataset_mnist(data_filename, label_filename);
	}
	
	void start()
	{
		if (_interval == std::chrono::milliseconds(0) || _callback == nullptr)
		{
			throw std::logic_error("interval or callback not set");
		}
		_thread.reset(new std::thread(&data_generator::worker_loop, this));
	}
	
	void stop()
	{
		_running = false;
		_thread->join();
	}
	
	~data_generator()
	{
		stop();
	}
	
	void set_generate_interval(std::chrono::milliseconds interval)
	{
		_interval = interval;
	}
	
	void set_generate_callback(generate_callback_type callback)
	{
		_callback = callback;
	}

	void set_generate_size(size_t size)
	{
		_generate_size = size;
	}
	
private:
	bool _running;
	size_t _generate_size;
	Ml::data_converter<DType> _dataset;
	std::shared_ptr<std::thread> _thread;
	std::chrono::milliseconds _interval;
	generate_callback_type _callback;
	
	void worker_loop()
	{
		while (_running)
		{
			auto data = _dataset.get_random_data(_generate_size);
			if (_callback) _callback(data);
			std::this_thread::sleep_for(_interval);
		}
	}
};