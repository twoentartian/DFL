//
// Created by jxzhang on 09-08-21.
//

#ifndef DFL_NODE_HPP
#define DFL_NODE_HPP

#include <optional>
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
	normal = 0,
	malicious_random_strategy,
	malicious_strategy_1,
	malicious_strategy_2,
	observer,
	
	node_type_last_index
};

const std::array<std::string, node_type_last_index> node_type_str{
		"normal",
		"malicious_random_strategy",
		"malicious_strategy_1",
		"malicious_strategy_2",
		"observer_node",
};

template<typename model_datatype>
class node
{
public:
	node(std::string _name, size_t buf_size) : name(std::move(_name)), next_train_tick(0), buffer_size(buf_size), dataset_mode(dataset_mode_type::unknown)
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
		reputation_output->open(output_path / (name + "_reputation.txt"));
	}
	
	std::string name;
	dataset_mode_type dataset_mode;
	node_type type;
	std::unordered_map<int, std::tuple<Ml::caffe_parameter_net<model_datatype>, float>> nets_record; //for delayed accuracy testing
	std::unordered_map<int, model_datatype> nets_accuracy_only_record; //for non-delayed accuracy testing
	int next_train_tick;
	std::unordered_map<int, std::tuple<float, float>> special_non_iid_distribution; //label:{min,max}
	std::vector<int> training_interval_tick;
	
	size_t buffer_size;
	std::vector<std::tuple<std::string, Ml::model_compress_type, Ml::caffe_parameter_net<model_datatype>>> parameter_buffer;
	std::shared_ptr<Ml::MlCaffeModel<model_datatype, caffe::SGDSolver>> solver;
	std::unordered_map<std::string, double> reputation_map;
	Ml::model_compress_type model_generation_type;
	float filter_limit;
	std::shared_ptr<std::ofstream> reputation_output;
	
	std::vector<node *> peers;
	
	virtual void train_model(const std::vector<Ml::tensor_blob_like<model_datatype>> &data, const std::vector<Ml::tensor_blob_like<model_datatype>> &label, bool display) = 0;
	
	virtual std::optional<Ml::caffe_parameter_net<model_datatype>> generate_model_sent() = 0;
	
};


template<typename model_datatype>
class normal_node : public node<model_datatype>
{
public:
	normal_node(std::string _name, size_t buf_size) : node<model_datatype>(_name, buf_size)
	{
		this->type = normal;
	};
	
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
class malicious_node_random_generate : public node<model_datatype>
{
public:
	malicious_node_random_generate(std::string _name, size_t buf_size) : node<model_datatype>(_name, buf_size)
	{
		this->type = malicious_random_strategy;
	};
	
	void train_model(const std::vector<Ml::tensor_blob_like<model_datatype>> &data, const std::vector<Ml::tensor_blob_like<model_datatype>> &label, bool display) override
	{
		this->solver->train(data, label, display);
	}
	
	std::optional<Ml::caffe_parameter_net<model_datatype>> generate_model_sent() override
	{
		Ml::caffe_parameter_net<model_datatype> output = this->solver->get_parameter();
		output.random(0, 0.0001);
		return {output};
	}
};

template<typename model_datatype>
class malicious_node_strategy_1 : public node<model_datatype>
{
	
	/**
	 * Strategy 1: use trained model and random model by turn
	 **/

public:
	
	malicious_node_strategy_1(std::string _name, size_t buf_size) : node<model_datatype>(_name, buf_size)
	{
		this->type = malicious_strategy_1;
		turn = 0;
	};
	
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
			output.random(-0.1, 0.1);
		}
		turn = (turn + 1) % 2;
		return {output};
	}
	
};

template<typename model_datatype>
class malicious_node_strategy_2 : public node<model_datatype>
{
	
	/**
	 * Strategy 1: use trained model (trained several times) and random model by turn
	 **/
public:
	malicious_node_strategy_2(std::string _name, size_t buf_size) : node<model_datatype>(_name, buf_size)
	{
		this->type = malicious_strategy_2;
		turn = 0;
	};
	
	int turn;
	
	void train_model(const std::vector<Ml::tensor_blob_like<model_datatype>> &data, const std::vector<Ml::tensor_blob_like<model_datatype>> &label, bool display) override
	{
		for (int i = 0; i <= 5; ++i)
		{
			this->solver->train(data, label, display);
		}
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
			output.random(-0.5, 0.5);
		}
		turn = (turn + 1) % 2;
		return {output};
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
	
	void train_model(const std::vector<Ml::tensor_blob_like<model_datatype>> &data, const std::vector<Ml::tensor_blob_like<model_datatype>> &label, bool display) override
	{
	
	}
	
	std::optional<Ml::caffe_parameter_net<model_datatype>> generate_model_sent() override
	{
		return {};
	}
	
};


#endif //DFL_NODE_HPP
