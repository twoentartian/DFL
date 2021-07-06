#include <thread>
#include <time_util.hpp>
#include <byte_buffer.hpp>
#include <time_util.hpp>
#include <thread_pool.hpp>
#include <measure_time.hpp>

int main()
{
	measure_time measureTime;
	measureTime.start();
	thread_pool pool;
	pool.set_delete_callback([&measureTime](int64_t thread_id){
		std::cout << "delete:" << thread_id << " at "  << measureTime.instant_measure_ms() << "ms" <<std::endl;
	});
	pool.insert_thread([](){
		std::this_thread::sleep_for(std::chrono::seconds(1));
	});
	pool.insert_thread([](){
		std::this_thread::sleep_for(std::chrono::seconds(2));
	});
	
	while (measureTime.instant_measure_ms() < 1500.0)
	{
		pool.auto_clean();
	}
	
	pool.wait_for_close();
	measureTime.stop();
	std::cout << "time:" << measureTime.measure_ms() << "ms" << std::endl;
}
