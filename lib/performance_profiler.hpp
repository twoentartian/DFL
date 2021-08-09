#pragma once

#include <unordered_map>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

class global_profiler_recorder
{
public:
	static global_profiler_recorder* instance()
	{
		if (_instance == nullptr)
		{
			static global_profiler_recorder temp_instance;
			_instance = &temp_instance;
		}
		return _instance;
	}
	
	struct record
	{
	public:
		record(const std::chrono::time_point<std::chrono::high_resolution_clock>& _start, const std::chrono::time_point<std::chrono::high_resolution_clock>& _stop) : start(_start), stop(_stop)
		{
			duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
		}
		
		std::chrono::time_point<std::chrono::high_resolution_clock> start;
		std::chrono::time_point<std::chrono::high_resolution_clock> stop;
		std::chrono::nanoseconds duration_ns;
	};
	
	std::unordered_map<std::string, std::vector<record>>& get_record()
	{
		return _performance_record;
	}
	
private:
	friend class profiler_abs;
	static global_profiler_recorder* _instance;
	std::unordered_map<std::string, std::vector<record>> _performance_record;
	
	using time_point = std::chrono::high_resolution_clock::time_point;
	std::string serializeTimePoint( const time_point& time, const std::string& format)
	{
		std::time_t tt = std::chrono::high_resolution_clock::to_time_t(time);
		std::tm tm = *std::gmtime(&tt); //GMT (UTC)
		//std::tm tm = *std::localtime(&tt); //Locale time-zone, usually UTC by default.
		std::stringstream ss;
		ss << std::put_time( &tm, format.c_str() );
		return ss.str();
	}
	
	global_profiler_recorder()
	{
		_performance_record.clear();
	}
	
	~global_profiler_recorder()
	{
		std::ofstream output_file;
		output_file.open("./performance_profiler.csv", std::ios::binary);
		if (!output_file.is_open())
		{
			std::cout << "failed to create performance profiler result" << std::endl;
			return;
		}
		std::map<std::string, uint64_t> summary;
		for (const auto& [name, records] : _performance_record)
		{
			auto [iter, status] = summary.emplace(name, 0);
			output_file << name << "(elapsed), (start), (stop)" << std::endl;
			for (const auto& single_record: records)
			{
				auto start_us = std::chrono::duration_cast<std::chrono::nanoseconds>(single_record.start.time_since_epoch()).count() % 1000000000;
				auto start_t_str = serializeTimePoint(single_record.start, "UTC: %F %T");
				auto stop_us = std::chrono::duration_cast<std::chrono::nanoseconds>(single_record.stop.time_since_epoch()).count() % 1000000000;
				auto stop_t_str = serializeTimePoint(single_record.stop, "UTC: %F %T");
				auto ns = single_record.duration_ns.count();
				iter->second += ns;
				output_file << ns << "," << start_t_str << "." << start_us << "," << stop_t_str << "." << stop_us << std::endl;
			}
			output_file << std::endl << std::endl;
		}
		
		//summary
		output_file << std::endl << std::endl;
		output_file << "summary" << std::endl;
		for (const auto& [name, ns] : summary)
		{
			output_file << name << ",";
		}
		output_file << std::endl;
		for (const auto& [name, ns] : summary)
		{
			output_file << ns << ",";
		}
		output_file << std::endl;
		
		output_file.flush();
		output_file.close();
	}
	
	void add_record(const std::string& name, const std::chrono::time_point<std::chrono::high_resolution_clock>& start, const std::chrono::time_point<std::chrono::high_resolution_clock>& stop)
	{
		if (_performance_record.find(name) == _performance_record.end())
		{
			_performance_record.emplace(name, std::vector<record>());
		}
		_performance_record[name].emplace_back(start,stop);
	}
};
global_profiler_recorder* global_profiler_recorder::_instance = nullptr;

class profiler_abs
{
public:
	profiler_abs(std::string name) : _name(std::move(name))
	{
		if (_instance == nullptr) _instance = global_profiler_recorder::instance();
	}
	
protected:
	void __start__()
	{
		start_time = std::chrono::high_resolution_clock::now();
	}
	
	void __stop__()
	{
		auto stop_time = std::chrono::high_resolution_clock::now();
		_instance->add_record(_name, start_time, stop_time);
	}

protected:
	static global_profiler_recorder* _instance;
	std::string _name;
	std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
};
global_profiler_recorder* profiler_abs::_instance = nullptr;

class profiler_auto : public profiler_abs
{
public:
	explicit profiler_auto(const std::string& name) : profiler_abs(name)
	{
		__start__();
	}
	
	~profiler_auto()
	{
		__stop__();
	}
};

class profiler_manual : public profiler_abs
{
public:
	explicit profiler_manual(const std::string& name) : profiler_abs(name)
	{
	
	}
	
	void start()
	{
		__start__();
	}
	
	void stop()
	{
		__stop__();
	}
};
