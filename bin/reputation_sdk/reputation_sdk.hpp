#pragma once

#include <unordered_map>
#include <string>

#include <glog/logging.h>

#include <dll_importer.hpp>
#include <ml_layer.hpp>

enum class model_type
{
	unknown = 0,
	normal,
	filtered,
	
	model_type_lastIndex
};

template<typename DType>
class updated_model
{
public:
	Ml::caffe_parameter_net<DType> model_parameter;
	model_type type;
	double accuracy;
	std::string generator_address;
};

template<typename DType>
class reputation_interface
{
public:
	virtual void update_model(Ml::caffe_parameter_net<DType>& current_model, double self_accuracy, const std::vector<updated_model<DType>>& models, std::unordered_map<std::string, double>& reputation) = 0;
	
};