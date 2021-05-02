//
// Created by gzr on 01-05-21.
//

#include <data_converter/data_converter.hpp>
#include <data_converter/caffe_data_convert.hpp>
#include <caffe/util/io.hpp>
#include <ml_layer/ml_abs.hpp>
#include <ml_layer/caffe.hpp>
#include <ml_layer/caffe_model_parameters.hpp>
#include <caffe/sgd_solvers.hpp>
int main()
{

    caffe::Caffe::set_mode(caffe::Caffe::CPU);
    Ml::MlCaffeModel<float> model1;
    model1.load_caffe_model<caffe::SGDSolver>("../../../dataset/MNIST/lenet_solver.prototxt");
    auto solver = model1.raw();
    const auto& net = solver->net();
    const auto& test_net = solver->test_nets();
    const auto train_input_layer = boost::dynamic_pointer_cast<caffe::MemoryDataLayer<float>>( net->layer_by_name("data") );
    int height = train_input_layer->height();
    int weight = train_input_layer->width();
    int channel = train_input_layer->channels();
    const std::string argv_test[] {"../../../dataset/MNIST/t10k-images.idx3-ubyte",
                                   "../../../dataset/MNIST/t10k-labels.idx1-ubyte",
                                   };
    const std::string argv_train[] { "../../../dataset/MNIST/train-images.idx3-ubyte",
                                     "../../../dataset/MNIST/train-labels.idx1-ubyte",
                                     };
    data_converter train;
    train.load_dataset_minist(argv_train[0].c_str(),argv_train[1].c_str());
    auto train_data_blobs = train.get_data();
    auto train_label_blobs= train.get_label();
    data_converter::caffe_data_convert<float> test;
    test.load_dataset_minist(argv_test[0].c_str(),argv_test[1].c_str());
    auto test_data_blobs = test.get_data();
    auto test_label_blobs = test.get_label();
    Ml::tensor_blob_like<float>train_data;
    Ml::tensor_blob_like<float>train_label;
    train_data.fromBlob(train_data_blobs.get()[0]);
    train_label.fromBlob(train_label_blobs.get()[0]);
    //model1.train(train_data,train_label);


}