#pragma once

#include <memory>
#include <mutex>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <caffe/solver.hpp>
#include <caffe/sgd_solvers.hpp>
#include <caffe/net.hpp>

#include "./ml_abs.hpp"
#include "./caffe_model_parameters.hpp"
#include "./caffe_solver_ext.hpp"
#include "../exception.hpp"

namespace Ml
{
	template <typename DType, template <typename> class SolverType>
	class MlCaffeModel : public MlModel<DType>
	{
	public:
		std::stringstream serialization(serialization_type type) override
		{
			std::stringstream output;
			if (!_caffe_solver)
			{
				throw std::logic_error("caffe model is not loaded yet, use load_caffe_model() to load a model first");
			}
			const auto& net = _caffe_solver->net();
			if (!net)
			{
				throw std::logic_error("empty net pointer");
			}
			
			Ml::caffe_parameter_net<DType> net_parameter;

			net_parameter.fromNet(*net);
			if (type == serialization_type::model_serialization_boost_binary)
			{
				boost::archive::binary_oarchive o_archive(output);
				o_archive << net_parameter;
			}
			else if (type == serialization_type::model_serialization_boost_text)
			{
				boost::archive::text_oarchive o_archive(output);
				o_archive << net_parameter;
			}
			else if (type == serialization_type::model_serialization_boost_xml)
			{
				THROW_NOT_IMPLEMENTED;
			}
			else if (type == serialization_type::model_serialization_json)
			{
				THROW_NOT_IMPLEMENTED;
			}
			else
			{
				THROW_NOT_IMPLEMENTED;
			}
			
			return output;
		}
		
		void deserialization(serialization_type type, std::stringstream & ss) override
		{
			Ml::caffe_parameter_net<DType> net_parameter;
			if (type == serialization_type::model_serialization_boost_binary)
			{
				boost::archive::binary_iarchive i_archive(ss);
				i_archive >> net_parameter;
			}
			else if (type == serialization_type::model_serialization_boost_text)
			{
				boost::archive::text_iarchive i_archive(ss);
				i_archive >> net_parameter;
			}
			else if (type == serialization_type::model_serialization_boost_xml)
			{
				THROW_NOT_IMPLEMENTED;
			}
			else if (type == serialization_type::model_serialization_json)
			{
				THROW_NOT_IMPLEMENTED;
			}
			else
			{
				THROW_NOT_IMPLEMENTED;
			}
			net_parameter.toNet(*(_caffe_solver->net()));
		}
		
		void load_caffe_model(const std::string& proto_file_path)
		{
			std::lock_guard guard(_model_lock);
			caffe::SolverParameter solver_param;
			if (!caffe::ReadProtoFromTextFile(proto_file_path.c_str(), &solver_param))
			{
				throw std::logic_error("parse caffe model fail");
			}
			//_caffe_solver.reset(new SolverType<DType>(solver_param));
			_caffe_solver.reset(new Ml::caffe_solver_ext<float, SolverType>(solver_param));
		}
		
		void train(const std::vector<tensor_blob_like<DType>>& data, const std::vector<tensor_blob_like<DType>>& label, bool display = true) override
		{
			std::lock_guard guard(_model_lock);
			_caffe_solver->TrainDataset(data, label, display);
		}
		
		DType evaluation(const std::vector<tensor_blob_like<DType>>& data, const std::vector<tensor_blob_like<DType>>& label) override
		{
			std::lock_guard guard(_model_lock);
			std::vector<std::tuple<DType,DType>> results = _caffe_solver->TestDataset(data, label);
			DType output_accuracy = 0;
			for (int i = 0; i < results.size(); ++i)
			{
				DType accuracy, loss;
				std::tie(accuracy, loss) = results[i];
				output_accuracy += accuracy;
			}
			return output_accuracy / results.size();
		}
		
		std::vector<tensor_blob_like<DType>> predict(const std::vector<tensor_blob_like<DType>>& data) override
		{
			std::lock_guard guard(_model_lock);
			std::vector<std::vector<tensor_blob_like<DType>>> results = _caffe_solver->PredictDataset(data);
			return results[0];
		}
		
		std::string get_network_structure_info() override
		{
			if (!_caffe_solver)
			{
				throw std::logic_error("caffe model is not loaded yet, use load_caffe_model() to load a model first");
			}
			const auto& net = _caffe_solver->net();
			if (!net)
			{
				throw std::logic_error("empty net pointer");
			}
			Ml::caffe_parameter_net<DType> net_parameter;
			net_parameter.fromNet(*net);
			caffe_net_structure structure(net_parameter);
			std::stringstream ss;
			{
				boost::archive::binary_oarchive o_archive(ss);
				o_archive << structure;
			}
			return ss.str();
		}
		
		model_type get_model_type() override
		{
			return model_type::model_type_caffe;
		}
		
		boost::shared_ptr<caffe::Solver<DType>> raw()
		{
			return _caffe_solver;
		}
		
		caffe_parameter_net<DType> get_parameter()
		{
			Ml::caffe_parameter_net<DType> output;
			output.fromNet(*getNet());
			return output;
		}
		
		void set_parameter(const caffe_parameter_net<DType>& parameter)
		{
			std::lock_guard guard(_model_lock);
			parameter.toNet(*getNet());
		}
		
		void set_parameter(const caffe_parameter_net<DType>&& parameter)
		{
			std::lock_guard guard(_model_lock);
			parameter.toNet(*getNet());
		}
		
		int get_iter() override
		{
			return _caffe_solver->iter();
		}
		
	private:
		boost::shared_ptr<caffe::Net<DType>> getNet()
		{
			if (!_caffe_solver)
			{
				throw std::logic_error("caffe model is not loaded yet, use load_caffe_model() to load a model first");
			}
			boost::shared_ptr<caffe::Net<DType>> net = _caffe_solver->net();
			if (!net)
			{
				throw std::logic_error("empty net pointer");
			}
			return net;
		}
		
		//boost::shared_ptr<caffe::Solver<DType>> _caffe_solver;
		boost::shared_ptr<Ml::caffe_solver_ext<DType, SolverType>> _caffe_solver;
		std::mutex _model_lock;
	};
}
