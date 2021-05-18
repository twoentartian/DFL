#pragma once

#include <sstream>
#include "tensor_blob_like.hpp"

namespace Ml
{
    enum model_type
    {
        model_type_not_set = 0,
        model_type_caffe,
        model_type_tensorflow,

        model_type_unknown
    };

    enum model_data_type
    {
        data_type_float = 0,
        data_type_double
    };

    enum serialization_type
    {
        model_serialization_not_set = 0,
        model_serialization_boost_binary,
        model_serialization_boost_text,
        model_serialization_boost_xml,
        model_serialization_json,

        model_serialization_unknown
    };
    
    template <typename DType>
    class MlModel
    {
    public:
        virtual std::stringstream serialization(serialization_type type) = 0;

        virtual void deserialization(serialization_type type, std::stringstream & ss) = 0;
        
        virtual void train(const std::vector<tensor_blob_like<DType>>& data, const std::vector<tensor_blob_like<DType>>& label, bool display = true) = 0;
        
        virtual DType evaluation(const std::vector<tensor_blob_like<DType>>& data, const std::vector<tensor_blob_like<DType>>& label) = 0;
        
        virtual std::string get_network_structure_info() = 0;

        virtual model_type get_model_type()
        {
            return model_type::model_type_not_set;
        }
        
        virtual int get_iter() = 0;

    private:
        model_type type;

    };

}
