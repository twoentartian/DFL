#include <reputation_sdk.hpp>

template<typename DType>
class reputation_implementation : public reputation_interface<DType>
{
public:
	void update_model(Ml::caffe_parameter_net<DType>& current_model, double self_accuracy, const std::vector<updated_model<DType>>& models, std::unordered_map<std::string, double>& reputation) override
	{
		////////********////////    put the reputation logic here    ////////********////////
		////////********  to update the model, directly change the "current_model"
		////////********  to update the reputation, directly use "reputation[node_address]" to change
		Ml::fed_avg_buffer<Ml::caffe_parameter_net<DType>> parameter_buffers(models.size());
		double average_accuracy = 0.0;
		for (int i = 0; i < models.size(); ++i)
		{
			average_accuracy += models[i].accuracy;
		}
		average_accuracy /= models.size();
		
		for (int i = 0; i < models.size(); ++i)
		{
			if(models[i].type == model_type::normal)
			{
				if (models[i].accuracy < average_accuracy)
				{
					reputation[models[i].generator_address] -= 1.0;
				}
				parameter_buffers.add(models[i].model_parameter);
			}
		}
		current_model = parameter_buffers.average();
	}
	
};

EXPORT_REPUTATION_API