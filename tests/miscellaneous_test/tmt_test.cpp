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
		                                    }, 100000);)
	
	const int DATA_SIZE = 1000;
	int* data = new int[DATA_SIZE];
	for (int i = 0; i < DATA_SIZE; ++i)
	{
		data[i] = i;
	}
	
	for (int i = 0; i < 10000; ++i)
	{
		MEASURE_TIME(tmt::ParallelExecution_StepIncremental([&lock](uint32_t index, uint32_t thread_index, int& data)
		                                                    {
			                                                    std::lock_guard guard(lock);
			                                                    std::cout << "now: " << index << "  data:" << data << std::endl;
		                                                    }, DATA_SIZE, data);)
	}

	
	return 0;
}