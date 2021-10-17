#pragma once

#include "./node.hpp"

//return: train_data,train_label
template<typename model_datatype>
std::tuple<std::vector<Ml::tensor_blob_like<model_datatype>>, std::vector<Ml::tensor_blob_like<model_datatype>>>
get_dataset_by_node_type(Ml::data_converter<model_datatype> &dataset, const node<model_datatype> &target_node, int size, const std::vector<int> &ml_dataset_all_possible_labels)
{
	Ml::tensor_blob_like<model_datatype> label;
	label.getShape() = {1};
	std::vector<Ml::tensor_blob_like<model_datatype>> train_data, train_label;
	if (target_node.dataset_mode == dataset_mode_type::default_dataset)
	{
		//iid dataset
		std::tie(train_data, train_label) = dataset.get_random_data(size);
	}
	else if (target_node.dataset_mode == dataset_mode_type::iid_dataset)
	{
		std::random_device dev;
		std::mt19937 rng(dev());
		std::uniform_int_distribution<int> distribution(0, int(ml_dataset_all_possible_labels.size()) - 1);
		for (int i = 0; i < size; ++i)
		{
			int label_int = ml_dataset_all_possible_labels[distribution(rng)];
			label.getData() = {model_datatype(label_int)};
			auto[train_data_slice, train_label_slice] = dataset.get_random_data_by_Label(label, 1);
			train_data.insert(train_data.end(), train_data_slice.begin(), train_data_slice.end());
			train_label.insert(train_label.end(), train_label_slice.begin(), train_label_slice.end());
		}
	}
	else if (target_node.dataset_mode == dataset_mode_type::non_iid_dataset)
	{
		//non-iid dataset
		std::random_device dev;
		std::mt19937 rng(dev());
		
		Ml::non_iid_distribution<model_datatype> label_distribution;
		for (auto &target_label : ml_dataset_all_possible_labels)
		{
			label.getData() = {model_datatype(target_label)};
			auto iter = target_node.special_non_iid_distribution.find(target_label);
			if (iter != target_node.special_non_iid_distribution.end())
			{
				auto[dis_min, dis_max] = iter->second;
				if (dis_min == dis_max)
				{
					label_distribution.add_distribution(label, dis_min);
				}
				else
				{
					std::uniform_real_distribution<model_datatype> distribution(dis_min, dis_max);
					label_distribution.add_distribution(label, distribution(rng));
				}
			}
			else
			{
				LOG(ERROR) << "cannot find the desired label";
			}
		}
		std::tie(train_data, train_label) = dataset.get_random_non_iid_dataset(label_distribution, size);
	}
	return {train_data, train_label};
}

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
