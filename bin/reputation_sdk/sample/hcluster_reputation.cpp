#include <algorithm>
#include <reputation_sdk.hpp>
#include <clustering/hierarchical_clustering.hpp>

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
		
		//reputation
		double min_accuracy = 1.0;
		for (int i = 0; i < models.size(); ++i)
		{
			if (models[i].accuracy < min_accuracy) min_accuracy = models[i].accuracy;
		}
		for (int i = 0; i < models.size(); ++i)
		{
			if (models[i].accuracy == min_accuracy)
			{
				reputation[models[i].generator_address] -= 0.01;
				if (reputation[models[i].generator_address] < 0) reputation[models[i].generator_address] = 0;
			}
		}
		
		//hierarchical clustering
		std::vector<size_t> pass_index;
		std::vector<size_t> non_pass_index;
		{
			std::vector<clustering::data_point_2d<float>> points;
			points.reserve(models.size());
			for (int i = 0; i < models.size(); ++i)
			{
				auto x = models[i].accuracy;
				auto y = reputation[models[i].generator_address];
				points.emplace_back(x, y);
			}
			float majority_ratio = 0.5;
			clustering::hierarchical_clustering temp(false, 1, majority_ratio);
			auto history = temp.process(points, 1);
			auto last_state = history[history.size() - 1];
			for (auto& operation : last_state.merge_operations)
			{
				if (operation.elements_index.size() >= models.size() * majority_ratio)
				{
					pass_index = operation.elements_index;
				}
			}
		}
		
		for (int i = 0; i < models.size(); ++i)
		{
			if (std::find(pass_index.begin(), pass_index.end(), i) == pass_index.end())
			{
				non_pass_index.push_back(i);
			}
		}

		if (pass_index.empty()) LOG(FATAL) << "[Reputation DLL]: impossible situation, consider logic bug.";
		
		std::vector<updated_model<DType>> passed_model;
		passed_model.reserve(pass_index.size());
		for (auto index : pass_index)
		{
			passed_model.push_back(models[index]);
		}
		DType centroid_accuracy = 0;
		double centroid_reputation = 0;
		for (auto& model : passed_model)
		{
			centroid_accuracy += model.accuracy;
			centroid_reputation += reputation[model.generator_address];
		}
		centroid_accuracy /= passed_model.size();
		centroid_reputation /= passed_model.size();
		LOG(INFO) << "[Reputation DLL]: centroid_accuracy:" << centroid_accuracy << ", centroid_reputation:" << centroid_reputation;
		for (auto& model : passed_model)
		{
			LOG(INFO) << "[Reputation DLL]: HCluster chosen model = accuracy: " << model.accuracy << "(" << model.generator_address << "-" << reputation[model.generator_address] << ")";
		}
		
		//add models with higher accuracy to the passed_model
		for (auto single_non_passed_index : non_pass_index)
		{
			if (models[single_non_passed_index].accuracy > centroid_accuracy)
			{
				LOG(INFO) << "[Reputation DLL]: better accuracy model = accuracy: " << models[single_non_passed_index].accuracy << "(" << models[single_non_passed_index].generator_address << "-" << reputation[models[single_non_passed_index].generator_address] << ")";
				passed_model.push_back(models[single_non_passed_index]);
			}
		}
		
		//calculate average
		double average_accuracy = 0.0;
		double average_reputation = 0.0;
		for (int i = 0; i < passed_model.size(); ++i)
		{
			average_accuracy += passed_model[i].accuracy;
			average_reputation += reputation[passed_model[i].generator_address];
		}
		average_accuracy /= passed_model.size();
		average_reputation /= passed_model.size();
		
		std::vector<double> weights;
		std::vector<double> weights_filtered;
		weights.reserve(passed_model.size());
		weights_filtered.reserve(passed_model.size());
		double total_weight = 0.0;
		
		for (int i = 0; i < passed_model.size(); ++i)
		{
			if (passed_model[i].type == Ml::model_compress_type::normal)
			{
				double temp_weight = (passed_model[i].accuracy) * (reputation_copy[passed_model[i].generator_address]);
				total_weight = total_weight + temp_weight;
				weights.push_back(temp_weight);
			}
			else if (passed_model[i].type == Ml::model_compress_type::compressed_by_diff)
			{
				double temp_weight = (passed_model[i].accuracy) * (reputation_copy[passed_model[i].generator_address]);
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
			parameter_buffers.add(passed_model[i].model_parameter * weights[i]);
		}
		for (int i = 0; i < weights_filtered.size(); i++)
		{
			parameter_buffers_filtered.add(passed_model[i].model_parameter * weights_filtered[i]);
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
