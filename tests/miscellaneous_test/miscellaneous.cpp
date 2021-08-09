#include <thread>
#include <cmath>
#include <time_util.hpp>
#include <byte_buffer.hpp>
#include <time_util.hpp>
#include <thread_pool.hpp>
#include <measure_time.hpp>
#include <ml_layer.hpp>
#include <configure_file.hpp>
#include <duplicate_checker.hpp>
#include <performance_profiler.hpp>

#define BOOST_TEST_MAIN

#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_SUITE (miscellaneous_test)
	
	BOOST_AUTO_TEST_CASE (time_converter_test)
	{
		auto now = time_util::get_current_utc_time();
		auto now_str = time_util::time_to_text(now);
		std::cout << "now str: " << now_str << std::endl;
		auto now_copy = time_util::text_to_time(now_str);
		BOOST_CHECK(now == now_copy);

	}
	
	BOOST_AUTO_TEST_CASE (thread_test)
	{
		std::thread temp([](){
			std::this_thread::sleep_for(std::chrono::seconds(5));
		});
		temp.detach();
	}
	
	BOOST_AUTO_TEST_CASE (thread_pool_test)
	{
		measure_time measureTime;
		measureTime.start();
		thread_pool pool;
		pool.set_delete_callback([](int64_t thread_id){
			std::cout << "delete:" << thread_id << std::endl;
		});
		pool.insert_thread([](){
			std::this_thread::sleep_for(std::chrono::seconds(5));
		});
		pool.insert_thread([](){
			std::this_thread::sleep_for(std::chrono::seconds(10));
		});
		pool.wait_for_close();
		measureTime.stop();
		std::cout << "time:" << measureTime.measure_ms() << "ms" << std::endl;
	}
	
	BOOST_AUTO_TEST_CASE (nan)
	{
		double nan_num = NAN;
		std::cout << nan_num/1 << nan_num +1 << std::endl;
	}
	
	BOOST_AUTO_TEST_CASE (configuration_vector)
	{
		configuration_file configuration;
		configuration_file::json data;
		data["1"] = configuration_file::json::array({0,1,2,3,4});
		std::string data_str = data.dump();
		
		configuration.LoadConfigurationData(data_str);
		auto raw_data = *configuration.get_vec<int>("1");
		for (int i = 0; i < 5; ++i)
		{
			BOOST_CHECK(raw_data[i] == i);
		}
	}
	
	BOOST_AUTO_TEST_CASE (duplicate_checker_test)
	{
		duplicate_checker<int> checker(5, 1);
		BOOST_CHECK(checker.find(1) == false);
		checker.add(1);
		BOOST_CHECK(checker.find(1) == true);
		std::this_thread::sleep_for(std::chrono::seconds(6));
		BOOST_CHECK(checker.find(1) == false);
	}
	
	BOOST_AUTO_TEST_CASE (perfrmance_profiler)
	{
		profiler_auto p1("p1");
		std::this_thread::sleep_for(std::chrono::seconds(5));
	}
	
BOOST_AUTO_TEST_SUITE_END()