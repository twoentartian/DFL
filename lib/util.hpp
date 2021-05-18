#pragma once

#include <vector>
#include <random>

#define GENERATE_GET(Var_name, get_name)    \
auto& get_name()                            \
{return Var_name;}                          \
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
}