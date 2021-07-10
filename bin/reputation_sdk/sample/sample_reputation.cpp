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
		
		Ml::caffe_parameter_net<DType> zero_parameter = models[0].model_parameter - models[0].model_parameter;
		zero_parameter.set_all(0);
		
		Ml::fed_avg_buffer<Ml::caffe_parameter_net<DType>> parameter_buffers(models.size());
		Ml::fed_avg_buffer<Ml::caffe_parameter_net<DType>> parameter_buffers_filtered(models.size());
		double average_accuracy = 0.0;
		double min_accuracy = 1.0;
		for (int i = 0; i < models.size(); ++i)
		{
			average_accuracy += models[i].accuracy;
			if (models[i].accuracy < min_accuracy) min_accuracy = models[i].accuracy;
		}
		average_accuracy /= models.size();
		
		int counter_normal = 0, counter_filtered = 0;
		for (int i = 0; i < models.size(); ++i)
		{
			if(models[i].type == model_type::normal)
			{
				counter_normal++;
				if (models[i].accuracy == min_accuracy)
				{
					reputation[models[i].generator_address] -= 1.0;
				}
				parameter_buffers.add(models[i].model_parameter);
			}
			else if (models[i].type == model_type::filtered)
			{
				counter_filtered++;
				if (models[i].accuracy == min_accuracy)
				{
					reputation[models[i].generator_address] -= 1.0;
				}
				parameter_buffers_filtered.add(models[i].model_parameter);
			}
			else
			{
				LOG(WARNING) << "[REPUTATION SDK] unknown model type";
			}
		}
		int counter_total = counter_normal + counter_filtered;
		Ml::caffe_parameter_net<DType> parameter_normal_part, parameter_filtered_part;
		if (counter_normal != 0)
		{
			parameter_normal_part = parameter_buffers.average() * (float(counter_normal)/float(counter_total));
		}
		else
		{
			parameter_normal_part = zero_parameter;
		}
		
		auto current_model_copy = current_model;
		current_model_copy.patch_weight(parameter_buffers_filtered.average_ignore());
		if (counter_filtered != 0)
		{
			parameter_filtered_part = current_model_copy * (float(counter_filtered)/float(counter_total));
		}
		else
		{
			parameter_filtered_part = zero_parameter;
		}
		auto parameter_new_all = parameter_normal_part + parameter_filtered_part;
		current_model = (current_model + parameter_new_all) / 2;
	}
	
};

EXPORT_REPUTATION_API