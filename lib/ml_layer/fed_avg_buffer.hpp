//
// Created by tyd.
//

#pragma once

#include <vector>
#include <optional>
#include "util.hpp"

namespace Ml
{
	template <typename T>
	class fed_avg_buffer
	{
	public:
		fed_avg_buffer(size_t size) : _size(size)
		{
			_data.resize(_size);
			_current_write_loc = 0;
			_current_size = 0;
		}
		
		void add(const T& target)
		{
			_data[_current_write_loc] = target;
			move_to_next(_current_write_loc);
			
			if (_current_size != _size) _current_size++;
		}
		
		T average()
		{
			T output = _data[0] - _data[0];
			int buffer_size = _current_size;
			for (int i = 0; i < buffer_size; ++i)
			{
				output = output + _data[i];
			}
			output = output / _current_size;
			return output;
		}
		
		GENERATE_GET(_data,getData);
		GENERATE_GET(_current_size,getSize);
		
	private:
		std::vector<T> _data;
		size_t _current_write_loc;
		size_t _current_size;
		size_t _size;
		
		void move_to_next(size_t& value)
		{
			value++;
			if (value == _size)
			{
				//reach the end of ring buffer
				value = 0;
			}
		}
	};
}
