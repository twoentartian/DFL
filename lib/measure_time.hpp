#pragma once

#include <chrono>

class measure_time
{
protected:
	std::chrono::time_point<std::chrono::high_resolution_clock> m_begin, m_end;

public:
	inline measure_time()
	{
		m_begin = std::chrono::high_resolution_clock::now();
	}
	
	inline void start()
	{
		m_begin = std::chrono::high_resolution_clock::now();
	}
	
	inline void stop()
	{
		m_end = std::chrono::high_resolution_clock::now();
	}
	
	inline long measure()
	{
		return std::chrono::duration_cast<std::chrono::nanoseconds>(m_end - m_begin).count();
	}
	
	inline double measure_ms()
	{
		return double (std::chrono::duration_cast<std::chrono::nanoseconds>(m_end - m_begin).count()) / 1000000;
	}
};

#define MEASURE_TIME_LOCAL(func) {measure_time timer;timer.start();func;timer.stop();std::cout << "Line: " << __LINE__ << " costs: " << timer.measure_ms() << " s." << std::endl;};
#define PP_CONCAT(A, B) PP_CONCAT_IMPL(A, B)
#define PP_CONCAT_IMPL(A, B) A##B
#define MEASURE_TIME(func) measure_time PP_CONCAT(timer,__LINE__);PP_CONCAT(timer,__LINE__).start();func;PP_CONCAT(timer,__LINE__).stop();std::cout << "Line: " << __LINE__ << " costs: " << PP_CONCAT(timer,__LINE__).measure_ms() << " s." << std::endl;
