#pragma once

#include <vector>
#include <random>

#define GENERATE_GET(Var_name, get_name)    \
auto& get_name()                            \
{return Var_name;}                          \
[[nodiscard]] const auto& get_name() const  \
{return Var_name;}

#define GENERATE_READ(Var_name, get_name)   \
[[nodiscard]] const auto& get_name() const  \
{return Var_name;}

namespace util
{
	template <typename T>
	std::vector<T> vector_concatenate(const std::vector<T>& a, const std::vector<T>& b)
	{
		std::vector<T> output;
		output.reserve( a.size() + b.size() );
		output.insert( output.end(), a.begin(), a.end() );
		output.insert( output.end(), b.begin(), b.end() );
		return std::move(output);
	}
	
	template <typename T>
	void vector_twin_shuffle(std::vector<T>& data0, std::vector<T>& data1)
	{
		assert(data0.size() == data1.size());
		auto n = data0.end() - data0.begin();
		for (size_t i=0; i<n; ++i)
		{
			std::random_device dev;
			std::mt19937 rng(dev());
			std::uniform_int_distribution<size_t> distribution(0,n-1);
			
			size_t random_number = distribution(rng);
			data0[i].swap(data0[random_number]);
			data1[i].swap(data1[random_number]);
		}
	}
	
	template <typename T>
	class bool_setter
	{
	public:
		explicit bool_setter(T& value) : _value(value)
		{
			_value = true;
		}
		
		~bool_setter() noexcept
		{
			_value = false;
		}
		
		bool_setter(const bool_setter&) = delete;
		bool_setter& operator=(const bool_setter&) = delete;
	
	private:
		T& _value;
	};
	
	std::string get_random_str(int length = 20)
	{
		std::string randomStr;
		std::random_device randomDevice;
		std::default_random_engine randomEngine(randomDevice());
		std::uniform_int_distribution<int> distribution(0, 10+26+26-1);
		
		for (int i = 0; i < length; i++)
		{
			const int tempValue = distribution(randomEngine);
			if (tempValue < 10)
			{
				randomStr.push_back('0' + tempValue);
				continue;
			}
			else if(tempValue < 36)
			{
				randomStr.push_back('a' + tempValue-10);
				continue;
			}
			else if (tempValue < 62)
			{
				randomStr.push_back('A' + tempValue-36);
				continue;
			}
			else
			{
				throw std::logic_error("Impossible path");
			}
		}
		return randomStr;
	}
	
}