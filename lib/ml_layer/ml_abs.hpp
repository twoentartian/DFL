#pragma once

#include <sstream>


namespace MlAbs
{
	enum ml_model_type
	{
		not_set = 0,
		caffe,
		tensorflow,
		
		unknown
	};
	
	class MlModel
	{
	public:
		virtual std::stringstream serialization() = 0;
		
		virtual void deserialization(const std::string& ss) = 0;
		
		virtual void train(/*TODO*/) = 0;
		
		virtual void evaluation() = 0;
		
		virtual std::string get_network_structure_info() = 0;
		
		virtual ml_model_type get_model_type()
		{
			return not_set;
		}
		
	private:
		ml_model_type type;
		
	};
	
}
