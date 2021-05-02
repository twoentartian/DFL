#pragma once
#include <sstream>
#include <vector>
#include <memory>
#include <caffe/caffe.hpp>
#include <caffe/solver.hpp>
#include <caffe/net.hpp>
#include <memory>
#include <caffe/layers/memory_data_layer.hpp>
#include "../exception.hpp"
#include <caffe/blob.hpp>

#include <ml_layer/ml_abs.hpp>

#include <util.hpp>
#include <tensor_blob_like.hpp>

namespace Ml{

    template <typename DType>
    class data_convert{
    public:
        virtual void load_dataset_minist(const std::string &image_filename,const std::string &label_filename);
        virtual const tensor_blob_like<DType>& get_data() const =0;
        virtual const tensor_blob_like<DType>& get_label() const =0;



    private:
        Ml::model_data_type type;
        tensor_blob_like<DType> data;
        tensor_blob_like<DType> label;


    };




}