#include <time_util.hpp>
#include <byte_buffer.hpp>
#include <time_util.hpp>

#define BOOST_TEST_MAIN

#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_SUITE (miscellaneous_test)
	
	BOOST_AUTO_TEST_CASE (byte_buffer_test)
	{
		uint64_t x = 473890;
		uint32_t y = 372189;
		std::string s = "asdwqe";
		
		byte_buffer buffers[2];
		for (int i = 0; i < 2; ++i)
		{
//		buffers[i].add(x);
//		buffers[i].add(y);
//		buffers[i].add(s);
			buffers[i] << x << y << s;
		}
		BOOST_CHECK((buffers[0].size() == buffers[1].size()) && (buffers[0].size() == 18));
		BOOST_CHECK(std::memcmp(buffers[0].data(), buffers[1].data(), buffers[0].size()) == 0);
	}
	
	BOOST_AUTO_TEST_CASE (time_converter_test)
	{
		auto now = time_util::get_current_utc_time();
		auto now_str = time_util::time_to_text(now);
		std::cout << "now str: " << now_str << std::endl;
		auto now_copy = time_util::text_to_time(now_str);
		BOOST_CHECK(now == now_copy);

	}


BOOST_AUTO_TEST_SUITE_END()