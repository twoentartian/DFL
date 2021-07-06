#include <thread>
#include <time_util.hpp>
#include <byte_buffer.hpp>
#include <time_util.hpp>
#include <thread_pool.hpp>
#include <measure_time.hpp>

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
	
	
BOOST_AUTO_TEST_SUITE_END()