#pragma once

#include <thread>
#include <functional>

namespace auto_multi_thread
{
	////////////////////////////////////////////
	//Internal Function, unsafe use.
	////////////////////////////////////////////
	
	template <class Function, typename Count, typename ...T>
	void ParallelExecutionFunc(uint32_t threadNo, uint32_t totalThread, Count count, Function func, T* ...data)
	{
		for (uint32_t index = static_cast<uint32_t>(count * threadNo / totalThread); index < static_cast<uint32_t>(count * (threadNo + 1) / totalThread); ++index)
		{
			func(index, data[index]...);
		}
	}
	
	/* Use:
	auto_multi_thread::ParallelExecution([](uint32_t index, Data& data...)
	{
		function body
	}, Count, data pointer...);
	 */
	template <class Function, typename Count, typename ...T>
	void ParallelExecution(uint32_t totalThread, Function func, Count count, T* ...data)
	{
		//uint32_t totalThread = std::thread::hardware_concurrency();
		std::thread* threadPool = new std::thread[totalThread];
		
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
	void ParallelExecutionFunc_with_thread_index(uint32_t threadNo, uint32_t totalThread, Count count, Function func, T* ...data)
	{
		for (uint32_t index = static_cast<uint32_t>(count * threadNo / totalThread); index < static_cast<uint32_t>(count * (threadNo + 1) / totalThread); ++index)
		{
			func(index, threadNo, data[index]...);
		}
	}
	
	/** Use:
	 * auto_multi_thread::ParallelExecution([](uint32_t index, uint32_t thread_index, Data& data...)
	 * {
	 * 		function body
	 * }, Count, data pointer...);
	 */
	template <class Function, typename Count, typename ...T>
	void ParallelExecution_with_thread_index(uint32_t totalThread, Function func, Count count, T* ...data)
	{
		//uint32_t totalThread = std::thread::hardware_concurrency();
		std::thread* threadPool = new std::thread[totalThread];
		
		for (uint32_t threadNo = 0; threadNo < totalThread; ++threadNo)
		{
			std::thread tempThread(std::bind(ParallelExecutionFunc_with_thread_index<Function, Count, T...>, threadNo, totalThread, count, func, data...));
			threadPool[threadNo].swap(tempThread);
		}
		for (uint32_t threadNo = 0; threadNo < totalThread; ++threadNo)
		{
			threadPool[threadNo].join();
		}
		delete[] threadPool;
	}
	
	template <class Function, typename Count, typename ...T>
	void ParallelExecution(Function func, Count count, T* ...data)
	{
		uint32_t totalThread = std::thread::hardware_concurrency();
		ParallelExecution(totalThread, func, count, data...);
	}
	
	////////////////////////////////////////////
	//ParallelExecution_ptr
	////////////////////////////////////////////
	
	//Internal Function, unsafe use.
	template <class Function, typename Count, typename ...T>
	void ParallelExecutionFunc_ptr(uint32_t threadNo, uint32_t totalThread, Count count, Function func, T* ...data)
	{
		for (uint32_t index = static_cast<uint32_t>(count * threadNo / totalThread); index < static_cast<uint32_t>(count * (threadNo + 1) / totalThread); ++index)
		{
			func(index, data...);
		}
	}
	
	/* Use:
	auto_multi_thread::ParallelExecution([](uint32_t index, Data* data...)
	{
		function body
	}, Count, data pointer...);
	 */
	template <class Function, typename Count, typename ...T>
	void ParallelExecution_ptr(Function func, Count count, T* ...data)
	{
		uint32_t totalThread = std::thread::hardware_concurrency();
		std::thread* threadPool = new std::thread[totalThread];
		
		for (uint32_t threadNo = 0; threadNo < totalThread; ++threadNo)
		{
			std::thread tempThread(std::bind(ParallelExecutionFunc_ptr<Function, Count, T...>, threadNo, totalThread, count, func, data...));
			threadPool[threadNo].swap(tempThread);
		}
		for (uint32_t threadNo = 0; threadNo < totalThread; ++threadNo)
		{
			threadPool[threadNo].join();
		}
		delete[] threadPool;
	}
}
