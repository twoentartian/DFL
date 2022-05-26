//
// Created by jxzhang on 09-08-21. Malicious types extended by tyd on 20-09-21
//

#ifndef DFL_NODE_HPP
#define DFL_NODE_HPP

#include <optional>
#include <random>
#include <set>
#include <vector>
#include <unordered_map>
#include "../../lib/ml_layer.hpp"

enum class dataset_mode_type
{
	unknown,
	default_dataset,
	iid_dataset,
	non_iid_dataset
};

enum node_type
{
	unknown_node_type = 0,
	normal,
	observer,
	malicious_model_poisoning_random_model,
	malicious_model_poisoning_random_model_by_turn,
	malicious_model_poisoning_random_model_biased_0_1,
	malicious_duplication_attack,
	malicious_data_poisoning_shuffle_label,
	malicious_data_poisoning_shuffle_label_biased_1,
	malicious_data_poisoning_random_data,
	
	node_type_last_index
};


template<typename model_datatype>
class node
{
public:
	node(std::string _name, size_t buf_size) : name(std::move(_name)), next_train_tick(0), buffer_size(buf_size), planned_buffer_size(buf_size), dataset_mode(dataset_mode_type::unknown), model_generation_type(Ml::model_compress_type::unknown), filter_limit(0.0f), last_measured_accuracy(0.0f), last_measured_tick(0), type(node_type::unknown_node_type)
	{
		solver.reset(new Ml::MlCaffeModel<model_datatype, caffe::SGDSolver>());
	}
	
	virtual ~node()
	{
		if (reputation_output)
		{
			reputation_output->flush();
			reputation_output->close();
		}
		std::cout << "solver " << solver.use_count() << std::endl;
		std::cout << "reputation_output" << reputation_output.use_count() << std::endl;
	}
	
	void open_reputation_file(const std::filesystem::path &output_path)
	{
		reputation_output.reset(new std::ofstream());
		reputation_output->open(output_path / (name + "_reputation.csv"));
	}
	
	std::string name;
	dataset_mode_type dataset_mode;
	node_type type;
	//std::unordered_map<int, std::tuple<Ml::caffe_parameter_net<model_datatype>, float>> nets_record; //for delayed accuracy testing
	std::unordered_map<int, model_datatype> nets_accuracy_only_record; //for non-delayed accuracy testing
	int next_train_tick;
	std::unordered_map<int, std::tuple<float, float>> special_non_iid_distribution; //label:{min,max}
	std::vector<int> training_interval_tick;
	
	size_t buffer_size;
	size_t planned_buffer_size;
	
	std::vector<std::tuple<std::string, Ml::model_compress_type, Ml::caffe_parameter_net<model_datatype>>> parameter_buffer;
	std::mutex parameter_buffer_lock;
	std::shared_ptr<Ml::MlCaffeModel<model_datatype, caffe::SGDSolver>> solver;
	std::unordered_map<std::string, double> reputation_map;
	Ml::model_compress_type model_generation_type;
	float filter_limit;
	std::shared_ptr<std::ofstream> reputation_output;
	
	std::unordered_map<std::string, node *> peers;
	std::unordered_map<std::string, node *> planned_peers;
	
	float last_measured_accuracy;
	int last_measured_tick;
	
	virtual void train_model(const std::vector<Ml::tensor_blob_like<model_datatype>> &data, const std::vector<Ml::tensor_blob_like<model_datatype>> &label, bool display) = 0;
	
	virtual std::optional<Ml::caffe_parameter_net<model_datatype>> generate_model_sent() = 0;
	
	virtual node<model_datatype> *new_node(std::string _name, size_t buf_size) = 0;
	
	static node<model_datatype> *get_node_by_type(const std::string type)
	{
		auto iter = RegisteredNodeType.find(type);
		if (iter == RegisteredNodeType.end())
		{
			return nullptr;
		}
		else
		{
			return iter->second;
		}
	}

private:
	static std::unordered_map<std::string, node<model_datatype> *> RegisteredNodeType;

protected:
	static void _registerNodeType(const std::string &name, node<model_datatype> *node)
	{
		auto iter = RegisteredNodeType.find(name);
		if (iter == RegisteredNodeType.end())
		{
			RegisteredNodeType.emplace(name, node);
		}
		else
		{
			delete node;
		}
	}
};

template<typename model_datatype> std::unordered_map<std::string, node<model_datatype> *> node<model_datatype>::RegisteredNodeType;

template<typename model_datatype>
class normal_node : public node<model_datatype>
{
public:
	normal_node(std::string _name, size_t buf_size) : node<model_datatype>(_name, buf_size)
	{
		this->type = normal;
	};
	
	static std::string type_name()
	{
		return "normal";
	}
	
	static void registerNodeType()
	{
		node<model_datatype>::_registerNodeType(type_name(), new normal_node("template", 0));
	}
	
	node<model_datatype> *new_node(std::string _name, size_t buf_size) override
	{
		return new normal_node(_name, buf_size);
	}
	
	void train_model(const std::vector<Ml::tensor_blob_like<model_datatype>> &data, const std::vector<Ml::tensor_blob_like<model_datatype>> &label, bool display) override
	{
		this->solver->train(data, label, display);
	}
	
	std::optional<Ml::caffe_parameter_net<model_datatype>> generate_model_sent() override
	{
		return {this->solver->get_parameter()};
	}
};

template<typename model_datatype>
class observer_node : public node<model_datatype>
{
public:
	observer_node(std::string _name, size_t buf_size) : node<model_datatype>(_name, buf_size)
	{
		this->type = observer;
	};
	
	static std::string type_name()
	{
		return "observer";
	}
	
	static void registerNodeType()
	{
		node<model_datatype>::_registerNodeType(type_name(), new observer_node("template", 0));
	}
	
	node<model_datatype> *new_node(std::string _name, size_t buf_size) override
	{
		return new observer_node(_name, buf_size);
	}
	
	void train_model(const std::vector<Ml::tensor_blob_like<model_datatype>> &data, const std::vector<Ml::tensor_blob_like<model_datatype>> &label, bool display) override
	{
	
	}
	
	std::optional<Ml::caffe_parameter_net<model_datatype>> generate_model_sent() override
	{
		return {};
	}
	
};

template<typename model_datatype>
class malicious_model_poisoning_random_model_node : public node<model_datatype>
{
public:
	malicious_model_poisoning_random_model_node(std::string _name, size_t buf_size) : node<model_datatype>(_name, buf_size)
	{
		this->type = malicious_model_poisoning_random_model;
	};
	
	static std::string type_name()
	{
		return "malicious_model_poisoning_random_model";
	}
	
	static void registerNodeType()
	{
		node<model_datatype>::_registerNodeType(type_name(), new malicious_model_poisoning_random_model_node("template", 0));
	}
	
	node<model_datatype> *new_node(std::string _name, size_t buf_size) override
	{
		return new malicious_model_poisoning_random_model_node(_name, buf_size);
	}
	
	void train_model(const std::vector<Ml::tensor_blob_like<model_datatype>> &data, const std::vector<Ml::tensor_blob_like<model_datatype>> &label, bool display) override
	{
		this->solver->train(data, label, display);
	}
	
	std::optional<Ml::caffe_parameter_net<model_datatype>> generate_model_sent() override
	{
		Ml::caffe_parameter_net<model_datatype> output = this->solver->get_parameter();
		
//		auto factor = output;
//		factor.random(0.7,1);
//		output = output.dot_product(factor);
		
		output.random(0,0.001);

		return {output};
	}
};

template<typename model_datatype>
class malicious_model_poisoning_random_model_by_turn_node : public node<model_datatype>
{
	/**
	 * Strategy: use trained model and random model by turn
	 **/
public:
	
	malicious_model_poisoning_random_model_by_turn_node(std::string _name, size_t buf_size) : node<model_datatype>(_name, buf_size)
	{
		this->type = malicious_model_poisoning_random_model_by_turn;
		turn = 0;
	};
	
	static std::string type_name()
	{
		return "malicious_model_poisoning_random_model_by_turn";
	}
	
	static void registerNodeType()
	{
		node<model_datatype>::_registerNodeType(type_name(), new malicious_model_poisoning_random_model_by_turn_node("template", 0));
	}
	
	node<model_datatype> *new_node(std::string _name, size_t buf_size) override
	{
		return new malicious_model_poisoning_random_model_by_turn_node(_name, buf_size);
	}
	
	int turn;
	
	void train_model(const std::vector<Ml::tensor_blob_like<model_datatype>> &data, const std::vector<Ml::tensor_blob_like<model_datatype>> &label, bool display) override
	{
		this->solver->train(data, label, display);
	}
	
	std::optional<Ml::caffe_parameter_net<model_datatype>> generate_model_sent() override
	{
		Ml::caffe_parameter_net<model_datatype> output;
		if (turn == 0)
		{
			output = this->solver->get_parameter();
		}
		else
		{
			output = this->solver->get_parameter();
//			auto factor = output;
//			factor.random(0.7,1);
//			output = output.dot_product(factor);
			output.random(0,0.001);
		}
		turn = (turn + 1) % 2;
		return {output};
	}
	
};

template<typename model_datatype>
class malicious_model_poisoning_random_model_biased_0_1_node : public node<model_datatype>
{
public:
	malicious_model_poisoning_random_model_biased_0_1_node(std::string _name, size_t buf_size) : node<model_datatype>(_name, buf_size)
	{
		this->type = malicious_model_poisoning_random_model_biased_0_1;
	};
	
	static std::string type_name()
	{
		return "malicious_model_poisoning_random_model_biased_0_1";
	}
	
	static void registerNodeType()
	{
		node<model_datatype>::_registerNodeType(type_name(), new malicious_model_poisoning_random_model_biased_0_1_node("template", 0));
	}
	
	node<model_datatype> *new_node(std::string _name, size_t buf_size) override
	{
		return new malicious_model_poisoning_random_model_biased_0_1_node(_name, buf_size);
	}
	
	void train_model(const std::vector<Ml::tensor_blob_like<model_datatype>> &data, const std::vector<Ml::tensor_blob_like<model_datatype>> &label, bool display) override
	{
		this->solver->train(data, label, display);
	}
	
	std::optional<Ml::caffe_parameter_net<model_datatype>> generate_model_sent() override
	{
		Ml::caffe_parameter_net<model_datatype> output = this->solver->get_parameter();
		auto factor = output;
		factor.random(0, 0.1);
		output = output - factor;
		//output.fix_nan();
		
		return {output};
	}
};

template<typename model_datatype>
class malicious_duplication_attack_node : public node<model_datatype>
{
public:
	malicious_duplication_attack_node(std::string _name, size_t buf_size) : node<model_datatype>(_name, buf_size)
	{
		this->type = malicious_duplication_attack;
	};
	
	static std::string type_name()
	{
		return "malicious_duplication_attack";
	}
	
	static void registerNodeType()
	{
		node<model_datatype>::_registerNodeType(type_name(), new malicious_duplication_attack_node("template", 0));
	}
	
	node<model_datatype> *new_node(std::string _name, size_t buf_size) override
	{
		return new malicious_duplication_attack_node(_name, buf_size);
	}
	
	void train_model(const std::vector<Ml::tensor_blob_like<model_datatype>> &data, const std::vector<Ml::tensor_blob_like<model_datatype>> &label, bool display) override
	{
	
	}
	
	std::optional<Ml::caffe_parameter_net<model_datatype>> generate_model_sent() override
	{
		Ml::caffe_parameter_net<model_datatype> output = this->solver->get_parameter();
		return {output};
	}
};

template<typename model_datatype>
class malicious_data_poisoning_shuffle_label_node : public node<model_datatype>
{
public:
	malicious_data_poisoning_shuffle_label_node(std::string _name, size_t buf_size) : node<model_datatype>(_name, buf_size)
	{
		this->type = malicious_data_poisoning_shuffle_label;
	};
	
	static std::string type_name()
	{
		return "malicious_data_poisoning_shuffle_label";
	}
	
	static void registerNodeType()
	{
		node<model_datatype>::_registerNodeType(type_name(), new malicious_data_poisoning_shuffle_label_node("template", 0));
	}
	
	node<model_datatype> *new_node(std::string _name, size_t buf_size) override
	{
		return new malicious_data_poisoning_shuffle_label_node(_name, buf_size);
	}
	
	void train_model(const std::vector<Ml::tensor_blob_like<model_datatype>> &data, const std::vector<Ml::tensor_blob_like<model_datatype>> &label, bool display) override
	{
		std::vector<Ml::tensor_blob_like<model_datatype>> label_duplicate = label;
		std::default_random_engine generator;
		std::uniform_int_distribution<int> dist(0, 9);
		for (int i = 0; i < label_duplicate.size(); ++i)
		{
			auto &labels = label_duplicate[i].getData();
			for (auto &value: labels)
			{
				value = dist(generator);
			}
		}
		this->solver->train(data, label_duplicate, display);
	}
	
	std::optional<Ml::caffe_parameter_net<model_datatype>> generate_model_sent() override
	{
		Ml::caffe_parameter_net<model_datatype> output = this->solver->get_parameter();
		return {output};
	}
};

template<typename model_datatype>
class malicious_data_poisoning_shuffle_label_biased_1_node : public node<model_datatype>
{
public:
	malicious_data_poisoning_shuffle_label_biased_1_node(std::string _name, size_t buf_size) : node<model_datatype>(_name, buf_size)
	{
		this->type = malicious_data_poisoning_shuffle_label_biased_1;
	};
	
	static std::string type_name()
	{
		return "malicious_data_poisoning_shuffle_label_biased_1";
	}
	
	static void registerNodeType()
	{
		node<model_datatype>::_registerNodeType(type_name(), new malicious_data_poisoning_shuffle_label_biased_1_node("template", 0));
	}
	
	node<model_datatype> *new_node(std::string _name, size_t buf_size) override
	{
		return new malicious_data_poisoning_shuffle_label_biased_1_node(_name, buf_size);
	}
	
	void train_model(const std::vector<Ml::tensor_blob_like<model_datatype>> &data, const std::vector<Ml::tensor_blob_like<model_datatype>> &label, bool display) override
	{
		std::vector<Ml::tensor_blob_like<model_datatype>> label_duplicate = label;
		for (int i = 0; i < label_duplicate.size(); ++i)
		{
			auto &labels = label_duplicate[i].getData();
			for (auto &&single_label : labels)
			{
				single_label++;
				if (single_label == 10) single_label = 0;
			}
		}
		this->solver->train(data, label_duplicate, display);
	}
	
	std::optional<Ml::caffe_parameter_net<model_datatype>> generate_model_sent() override
	{
		Ml::caffe_parameter_net<model_datatype> output = this->solver->get_parameter();
		return {output};
	}
};

template<typename model_datatype>
class malicious_data_poisoning_random_data_node : public node<model_datatype>
{
public:
	malicious_data_poisoning_random_data_node(std::string _name, size_t buf_size) : node<model_datatype>(_name, buf_size)
	{
		this->type = malicious_data_poisoning_random_data;
	};
	
	static std::string type_name()
	{
		return "malicious_data_poisoning_random_data";
	}
	
	static void registerNodeType()
	{
		node<model_datatype>::_registerNodeType(type_name(), new malicious_data_poisoning_random_data_node("template", 0));
	}
	
	node<model_datatype> *new_node(std::string _name, size_t buf_size) override
	{
		return new malicious_data_poisoning_random_data_node(_name, buf_size);
	}
	
	void train_model(const std::vector<Ml::tensor_blob_like<model_datatype>> &data, const std::vector<Ml::tensor_blob_like<model_datatype>> &label, bool display) override
	{
		std::vector<Ml::tensor_blob_like<model_datatype>> data_duplicate = data;
		for (int i = 0; i < data_duplicate.size(); ++i)
		{
			data_duplicate[i].random(0.0, 1.0);
		}
		this->solver->train(data_duplicate, label, display);
	}
	
	std::optional<Ml::caffe_parameter_net<model_datatype>> generate_model_sent() override
	{
		Ml::caffe_parameter_net<model_datatype> output = this->solver->get_parameter();
		return {output};
	}
};


#endif //DFL_NODE_HPP
