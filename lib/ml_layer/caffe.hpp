#pragma once

#include <memory>
#include "./ml_abs.hpp"
#include "caffe/solver.hpp"

namespace MlAbs
{
	template <typename DType>
	class MlCaffe : MlModel
	{
	public:
		std::stringstream serialization()
		{
		
		}
		
		void deserialization(const std::string& ss)
		{
		
		}
		
		void train()
		{
		
		}
		
		void evaluation()
		{
		
		}
		
		//TODO: only return the network info string, used to calculate hash later.
		std::string get_network_structure_info()
		{
		
		}
		
		ml_model_type get_model_type() override
		{
			return caffe;
		}
		
	private:
		std::shared_ptr<caffe::Solver<DType>> _caffe_solver;
	};
	
	
	
}
