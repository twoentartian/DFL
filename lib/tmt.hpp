#pragma once

#include <functional>
#include <thread>
#include <mutex>
#include <atomic>

class tmt
{
public:
	/** Use:
	 *
     tmt::ParallelExecution([](uint32_t index, uint32_t thread_index, Data& data...)
     {
         function body
     }, Count, data pointer...);
     *
     tmt::ParallelExecution(uint32_t worker, [](uint32_t index, uint32_t thread_index, Data& data...)
     {
         function body
     }, Count, data pointer...);
     */
	template <class Function, typename Count, typename ...T>
	static void ParallelExecution(Function func, Count count, T* ...data)
	{
		uint32_t totalThread = std::thread::hardware_concurrency();
		ParallelExecution(totalThread, func, count, data...);
	}
	
	template <class Function, typename Count, typename ...T>
	static void ParallelExecution(uint32_t totalThread, Function func, Count count, T* ...data)
	{
		auto* threadPool = new std::thread[totalThread];
		
		for (uint32_t threadNo = 0; threadNo < totalThread; ++threadNo)
		{
			std::thread tempThread(std::bind(ParallelExecutionFunc<Function, Count, T...>, threadNo, totalThread, count, func, data...));
			threadPool[threadNo].swap(tempThread);
		}
		for (uint32_t threadNo = 0; threadNo < totalThread; ++threadNo)
		{
			threadPool[threadNo].join();
		}
		delete[] threadPool;
	}
	
	template <class Function, typename Count, typename ...T>
	static void ParallelExecution_StepIncremental(Function func, Count count, T* ...data)
	{
		uint32_t totalThread = std::thread::hardware_concurrency();
		ParallelExecution_StepIncremental(totalThread, func, count, data...);
	}
	
	template <class Function, typename Count, typename ...T>
	static void ParallelExecution_StepIncremental(uint32_t totalThread, Function func, Count count, T* ...data)
	{
		auto* threadPool = new std::thread[totalThread];
		std::mutex lock;
		std::atomic_uint32_t index = 0;
		for (uint32_t threadNo = 0; threadNo < totalThread; ++threadNo)
		{
			std::thread tempThread(std::bind(ParallelExecutionFunc_Step_incremental<Function, Count, T...>, threadNo, totalThread, std::ref(lock), std::ref(index), count, func, data...));
			//std::thread tempThread(std::bind(ParallelExecutionFunc<Function, Count, T...>, threadNo, totalThread, count, func, data...));
			threadPool[threadNo].swap(tempThread);
		}
		for (uint32_t threadNo = 0; threadNo < totalThread; ++threadNo)
		{
			threadPool[threadNo].join();
		}
		delete[] threadPool;
	}
	
private:
	template <class Function, typename Count, typename ...T>
	static void ParallelExecutionFunc(uint32_t threadNo, uint32_t totalThread, Count count, Function func, T* ...data)
	{
		for (auto index = static_cast<uint32_t>(count * threadNo / totalThread); index < static_cast<uint32_t>(count * (threadNo + 1) / totalThread); ++index)
		{
			func(index, threadNo, data[index]...);
		}
	}
	
	template <class Function, typename Count, typename ...T>
	static void ParallelExecutionFunc_Step_incremental(uint32_t threadNo, uint32_t totalThread, std::mutex& lock, std::atomic_uint32_t& index, uint32_t count, Function func, T* ...data)
	{
		lock.lock();
		while (index < count)
		{
			const int current_index = int (index);
			index++;
			lock.unlock();
			func(current_index, threadNo, data[current_index]...);
			lock.lock();
		}
		lock.unlock();
	}
};