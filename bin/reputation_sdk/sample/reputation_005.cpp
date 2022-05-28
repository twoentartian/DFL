#include <reputation_sdk.hpp>

template<typename DType>
class reputation_implementation : public reputation_interface<DType>
{
public:
	void update_model(Ml::caffe_parameter_net<DType> &current_model, double self_accuracy, const std::vector<updated_model<DType>> &models, std::unordered_map<std::string, double> &reputation) override
	{
		////////********////////    put the reputation logic here    ////////********////////
		////////********  to update the model, directly change the "current_model"
		////////********  to update the reputation, directly use "reputation[node_address]" to change
		
		Ml::caffe_parameter_net<DType> zero_parameter = models[0].model_parameter - models[0].model_parameter;
		zero_parameter.set_all(0);
		std::unordered_map<std::string, double> reputation_copy = reputation;
		
		//calculate average accuracy
		Ml::fed_avg_buffer<Ml::caffe_parameter_net<DType>> parameter_buffers(models.size());
		Ml::fed_avg_buffer<Ml::caffe_parameter_net<DType>> parameter_buffers_filtered(models.size());
		double average_accuracy = 0.0;
		double min_accuracy = 1.0;
		double average_reputation = 0.0;
		for (int i = 0; i < models.size(); ++i)
		{
			average_accuracy += models[i].accuracy;
			average_reputation += reputation[models[i].generator_address];
			if (models[i].accuracy < min_accuracy) min_accuracy = models[i].accuracy;
		}
		average_accuracy /= models.size();
		average_reputation /= models.size();
		
		std::vector<double> weights;
		std::vector<double> weights_filtered;
		weights.reserve(models.size());
		weights_filtered.reserve(models.size());
		double total_weight = 0.0;
		
		for (int i = 0; i < models.size(); ++i)
		{
			if (models[i].accuracy == min_accuracy)
			{
				reputation[models[i].generator_address] -= 0.05;
				if (reputation[models[i].generator_address] < 0) reputation[models[i].generator_address] = 0;
			}
			
			if (models[i].type == Ml::model_compress_type::normal)
			{
				double temp_weight = (models[i].accuracy) * (reputation_copy[models[i].generator_address]);
				total_weight = total_weight + temp_weight;
				weights.push_back(temp_weight);
			}
			else if (models[i].type == Ml::model_compress_type::compressed_by_diff)
			{
				double temp_weight = (models[i].accuracy) * (reputation_copy[models[i].generator_address]);
				total_weight = total_weight + temp_weight;
				weights_filtered.push_back(temp_weight);
			}
			else
			{
				LOG(WARNING) << "[REPUTATION SDK] unknown model type";
			}
		}
		
		//all weights are zero
		if (total_weight == 0.0)
		{
			for (auto&& single_weight : weights)
			{
				single_weight = 1.0;
				total_weight += single_weight;
			}
			for (auto&& single_weight : weights_filtered)
			{
				single_weight = 1.0;
				total_weight += single_weight;
			}
		}
		
		for (int i = 0; i < weights.size(); i++)
		{
			if (total_weight == 0.0)
			{
				weights[i] = 1;
			}
			else
			{
				weights[i] = weights[i] / total_weight * double (weights.size());
			}
		}
		for (int i = 0; i < weights_filtered.size(); i++)
		{
			if (total_weight == 0.0)
			{
				weights_filtered[i] = 1;
			}
			else
			{
				weights_filtered[i] = weights_filtered[i] / total_weight * double (weights_filtered.size());
			}
			
		}
		
		for (int i = 0; i < weights.size(); i++)
		{
			parameter_buffers.add(models[i].model_parameter * weights[i]);
		}
		for (int i = 0; i < weights_filtered.size(); i++)
		{
			parameter_buffers_filtered.add(models[i].model_parameter * weights_filtered[i]);
		}
		
		size_t counter_total = weights.size() + weights_filtered.size();
		Ml::caffe_parameter_net<DType> parameter_normal_part, parameter_filtered_part;
		if (!weights.empty())
		{
			parameter_normal_part = parameter_buffers.average() * (double (weights.size()) / double (counter_total));
		}
		else
		{
			parameter_normal_part = zero_parameter;
		}
		
		if (!weights_filtered.empty())
		{
			auto current_model_copy = current_model;
			current_model_copy.patch_weight(parameter_buffers_filtered.average_ignore());
			parameter_filtered_part = current_model_copy * (double (weights_filtered.size()) / double (counter_total));
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
