#include <boost/config.hpp>
#include <iostream>
#include "dll_interface.hpp"

class test_interface : public interface {
public:
	test_interface() {
		std::cout << "constructing" << std::endl;
	}
	
	std::string name() const {
		return "sum";
	}
	
	float calculate(float x, float y) {
		return x + y;
	}
	
	~test_interface() {
		std::cout << "destructing" << std::endl;
	}
};

extern "C" BOOST_SYMBOL_EXPORT test_interface plugin;
test_interface plugin;

