#pragma once

#include <string>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

class time_util
{
public:
	static time_t get_current_utc_time()
	{
		std::chrono::time_point currently = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now());
		std::chrono::duration seconds_since_utc_epoch = currently.time_since_epoch();
		return seconds_since_utc_epoch.count();
	}
	
	static std::string time_to_text(time_t utc_time)
	{
		std::time_t temp = utc_time;
		std::tm* t = std::gmtime(&temp);
		std::stringstream ss;
		ss << std::put_time(t, "%Y-%m-%d_%H:%M:%S_%Z");
		return ss.str();
	}
	
	static time_t text_to_time(const std::string& text)
	{
		std::tm tmp_time;
		strptime(text.c_str(),"%Y-%m-%d_%H:%M:%S_%Z",&tmp_time);
		std::time_t t = timegm(&tmp_time);
		return t;
	}

};