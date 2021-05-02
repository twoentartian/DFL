#pragma once

#include <memory>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <caffe/caffe.hpp>
#include <caffe/solver.hpp>
#include <caffe/net.hpp>
#include <memory>
#include <caffe/layers/memory_data_layer.hpp>
#include "./ml_abs.hpp"
#include "./caffe_model_parameters.hpp"
#include "../exception.hpp"


namespace Ml
{
    template <typename DType>
    class MlCaffeModel: MlModel
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

        template <template <typename> class SolverType>
        void load_caffe_model(const std::string& proto_file_path)
        {
            caffe::SolverParameter solver_param;
            if (!caffe::ReadProtoFromTextFile(proto_file_path.c_str(), &solver_param))
            {
                throw std::logic_error("parse caffe model fail");
            }
            _caffe_solver.reset(new SolverType<DType>(solver_param));
        }

        void train()
        {


//            int train_data_size = train_data_blob.getData().size();
//            auto *train_data = train_data_blob.getData();
//            auto *label_data = train_label_blob.getData();
//
//            const auto& net = _caffe_solver->net();
//            const auto train_input_layer = boost::dynamic_pointer_cast<caffe::MemoryDataLayer<DType>>( net->layer_by_name("data") );
//            train_input_layer->Reset((DType*)train_data , (DType*)label_data, train_data_size);
//            LOG(INFO) << "Solve start.";
//            _caffe_solver->Solve();

        }

        void evaluation()
        {

//            const auto& test_net = _caffe_solver->test_nets();
//            caffe::Blob<DType> data;
//
//            int test_data_size = test_data_blob.getData().size();
//            auto *test_data = test_data_blob.getData();
//            auto *test_label= test_label_blob.getData();
//            const auto test_input_layer = boost::dynamic_pointer_cast<caffe::MemoryDataLayer<DType>>(test_net->layer_by_name("data"));
//            test_input_layer->Reset((DType*)test_data , (DType*)test_label, test_data_size);

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


        std::shared_ptr<caffe::Solver<DType>> raw()
        {
            return _caffe_solver;
        }

        caffe_parameter_net<DType> get_parameter()
        {
            Ml::caffe_parameter_net<DType> output;
            output.fromNet(*getNet());
            return output;
        }

        void set_parameter(caffe_parameter_net<DType>& parameter)
        {
            parameter.toNet(*getNet());
        }

    private:
        const auto& getNet()
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
            return net;
        }

        std::shared_ptr<caffe::Solver<DType>> _caffe_solver;
    };

}
