#pragma once


//return <max, min>
template<typename T>
std::tuple<T, T> find_max_min(T* data, size_t size)
{
	T max, min;
	max=*data;min=*data;
	for (int i = 0; i < size; ++i)
	{
		if (data[i] > max) max = data[i];
		if (data[i] < min) min = data[i];
	}
	return {max,min};
}
