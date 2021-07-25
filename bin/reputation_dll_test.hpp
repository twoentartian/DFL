#pragma once

#include <tuple>
#include <string>
#include <random>

#include "reputation_sdk.hpp"

template<typename DType>
std::tuple<bool, std::string> reputation_dll_same_reputation_test(dll_loader<reputation_interface<DType>>& reputation_dll, Ml::caffe_parameter_net<DType> net)
{
	constexpr int node_count = 5;
	constexpr double accuracy = 0.8;
	constexpr double reputation = 0.95;
	
	auto zero_net = net - net;
	
	//self weight
	DType self_weight;
	{
		net.random(-0.5, 0.5);
		auto parameter = net;
		auto sum = zero_net;
		double self_accuracy = accuracy;
		std::unordered_map<std::string, double> reputation_map;
		
		std::vector<updated_model<DType>> received_models;
		received_models.resize(node_count);
		for (int i = 0; i < node_count; ++i)
		{
			received_models[i].accuracy = accuracy;
			received_models[i].type = Ml::model_compress_type::normal;
			received_models[i].generator_address = std::to_string(i);
			received_models[i].model_parameter = zero_net;
			reputation_map[std::to_string(i)] = reputation;
		}
		
		reputation_dll.get()->update_model(parameter, self_accuracy, received_models, reputation_map);
		auto ratio_net = parameter.dot_divide(net);
		self_weight = ratio_net.sum() / ratio_net.size();
	}
	
	//weight for other nodes
	DType other_weight;
	{
		auto parameter = zero_net;
		auto sum = zero_net;
		double self_accuracy = accuracy;
		std::unordered_map<std::string, double> reputation_map;
		
		std::vector<updated_model<DType>> received_models;
		received_models.resize(node_count);
		for (int i = 0; i < node_count; ++i)
		{
			net.random(-0.5, 0.5);
			received_models[i].accuracy = accuracy;
			received_models[i].type = Ml::model_compress_type::normal;
			received_models[i].generator_address = std::to_string(i);
			received_models[i].model_parameter = net;
			reputation_map[std::to_string(i)] = reputation;
		}
		
		reputation_dll.get()->update_model(parameter, self_accuracy, received_models, reputation_map);
		
		//check parameter == average(parameter + received_models)
		for (int i = 0; i < node_count; ++i)
		{
			sum = sum + received_models[i].model_parameter;
		}
		sum = sum / (node_count);
		auto ratio_net = parameter.dot_divide(sum);
		other_weight = ratio_net.sum() / ratio_net.size();
	}
	
	return { std::abs(self_weight + other_weight - 1) < 0.01 , "weight: self - " + std::to_string(self_weight) + "  other - " + std::to_string(other_weight)};
}

template<typename DType>
std::tuple<bool, std::string> reputation_dll_same_model_test(dll_loader<reputation_interface<DType>>& reputation_dll, Ml::caffe_parameter_net<DType> net)
{
	std::random_device rd;
	std::uniform_real_distribution distribution(0.1,0.9);
	
	constexpr int node_count = 5;
	
	net.random(-0.5, 0.5);
	auto parameter = net;
	auto standard = parameter;
	double self_accuracy = distribution(rd);
	std::unordered_map<std::string, double> reputation_map;
	
	std::vector<updated_model<DType>> received_models;
	received_models.resize(node_count);
	for (int i = 0; i < node_count; ++i)
	{
		received_models[i].accuracy = distribution(rd);
		received_models[i].type = Ml::model_compress_type::normal;
		received_models[i].generator_address = std::to_string(i);
		received_models[i].model_parameter = net;
		reputation_map[std::to_string(i)] = distribution(rd);
	}
	
	reputation_dll.get()->update_model(parameter, self_accuracy, received_models, reputation_map);
	
	//check parameter == average(parameter + received_models)
	auto ratio = parameter.dot_divide(standard);
	net.set_all(1.0);
	auto one_net = net;
	DType expansion_ratio = ratio.sum() / ratio.size();
	return {std::abs(expansion_ratio - 1.0) < 0.001, "typical expansion factor: " + std::to_string(expansion_ratio)};
}