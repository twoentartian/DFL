#include <boost/config.hpp>
#include <string>

class interface
{
public:
	virtual std::string name() const = 0;
	virtual float calculate(float x, float y) = 0;
	
	virtual ~interface() {}
};
