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
			for (int i = 0; i < _current_size; ++i)
			{
				output = output + _data[i];
			}
			output = output / _current_size;
			return output;
		}
		
		T average_ignore(typename T::DataType ignore = NAN)
		{
			static_assert(std::is_same_v<T, caffe_parameter_net<typename T::DataType>>);
			caffe_parameter_net<typename T::DataType> output = _data[0] - _data[0], counter = _data[0] - _data[0];
			output.set_all(0);
			counter.set_all(0);
			
			if (_current_size == 0)
			{
				//error because there is no data in the buffer
				throw std::logic_error("no data is in the buffer");
			}
			
			for (size_t model_index = 0; model_index < _current_size; ++model_index)
			{
				auto& model = _data[model_index];
				for (size_t layer_index = 0; layer_index < model.getLayers().size(); ++layer_index )
				{
					auto& layer = model.getLayers()[layer_index];
					auto& layer_output = output.getLayers()[layer_index];
					auto& layer_counter = counter.getLayers()[layer_index];
					for (size_t weight_index = 0; weight_index < layer.getBlob_p()->getData().size(); ++weight_index)
					{
						auto& blob_data = layer.getBlob_p()->getData()[weight_index];
						auto& blob_data_output = layer_output.getBlob_p()->getData()[weight_index];
						auto& blob_data_counter = layer_counter.getBlob_p()->getData()[weight_index];
						
						if (blob_data == ignore) continue;
						blob_data_output += blob_data;
						blob_data_counter ++;
					}
				}
			}
			output = output.dot_divide(counter);
			
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
