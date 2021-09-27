#include <tmt.hpp>
#include <iostream>
#include <measure_time.hpp>

int main()
{
	std::mutex lock;
	MEASURE_TIME(tmt::ParallelExecution([&lock](uint32_t index, uint32_t thread_index)
		                                    {
			                                    std::lock_guard guard(lock);
			                                    //std::cout << "now: " << index << std::endl;
		                                    }, 10000000);)
	
	MEASURE_TIME(tmt::ParallelExecution_StepIncremental([&lock](uint32_t index, uint32_t thread_index)
		                                                    {
			                                                    std::lock_guard guard(lock);
			                                                    //std::cout << "now: " << index << std::endl;
		                                                    }, 10000000);)
	
	return 0;
}