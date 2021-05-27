#pragma once

#include <caffe/caffe.hpp>

namespace debug_tool
{
	template <typename DType>
	void mnist_print_digit(DType* src)
	{
		for (int j = 0; j < 28; ++j)
		{
			for (int k = 0; k < 28; ++k)
			{
				if constexpr(std::is_same_v<typename std::decay<DType>::type, float> || std::is_same_v<typename std::decay<DType>::type, double>)
					std::cout << ((src[k + j * 28] > 0) ? '*':'_');
				else
					std::cout << ((src[k + j * 28] != 0) ? '*':'_');
			}
			std::cout << std::endl;
		}
	}
	
	void print_mnist_datum(const caffe::Datum& datum)
	{
		const std::string datum_data_str = datum.data();
		mnist_print_digit(datum.data().data());
		std::cout << "label: " << datum.label() << std::endl;
	}
	
}

