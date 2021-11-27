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
        
        virtual std::vector<tensor_blob_like<DType>> predict(const std::vector<tensor_blob_like<DType>>& data) = 0;
        
        virtual std::string get_network_structure_info() = 0;

        virtual model_type get_model_type()
        {
            return model_type::model_type_not_set;
        }
        
        virtual int get_iter() = 0;
	
	    static std::vector<Ml::tensor_blob_like<DType>> get_labels_from_probability(const std::vector<Ml::tensor_blob_like<DType>>& prob)
	    {
		    std::vector<Ml::tensor_blob_like<DType>> output;
		    output.reserve(prob.size());
		    for (int i = 0; i < prob.size(); ++i)
		    {
			    DType max_prob = 0;
			    int max_prob_label = 0;
			    for (int label_index = 0; label_index < prob[i].size(); ++label_index)
			    {
				    const Ml::tensor_blob_like<DType>& tensorBlobLike = prob[i];
				    DType value = tensorBlobLike.getData()[label_index];
				    if (value > max_prob)
				    {
					    max_prob = value;
					    max_prob_label = label_index;
				    }
			    }
			    Ml::tensor_blob_like<DType> label;
			    label.getShape() = {1};
			    label.getData() = {DType(max_prob_label)};
			    output.push_back(label);
		    }
		    return output;
	    }
	    
	    static int count_correct_based_on_prediction(const std::vector<Ml::tensor_blob_like<DType>>& real_y, const std::vector<Ml::tensor_blob_like<DType>>& predict_y)
	    {
		    int correct_count = 0;
		    for (int i = 0; i < real_y.size(); ++i)
		    {
			    if (predict_y[i].roughly_equal(real_y[i], 0.1))
			    {
				    correct_count++;
			    }
		    }
		    return correct_count;
	    }


    private:
        model_type type;

    };

}
