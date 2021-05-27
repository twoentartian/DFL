//
// Created by gzr on 01-05-21.
//

#define CPU_ONLY
//#define _GLIBCXX_USE_CXX11_ABI 0
#include <ml_layer/data_convert.hpp>
#include <caffe/util/io.hpp>
#include <ml_layer/ml_abs.hpp>
#include <ml_layer/caffe.hpp>
#include <caffe/sgd_solvers.hpp>


int main() {


     caffe::Caffe::set_mode(caffe::Caffe::CPU);
//    Ml::MlCaffeModel<float> model1;
//    model1.load_caffe_model<caffe::SGDSolver>("../../../dataset/MNIST/lenet_solver.prototxt");
//    auto solver = model1.raw();
//    const auto& net = solver->net();
//    const auto& test_net = solver->test_nets();
////    //const auto train_input_layer = boost::dynamic_pointer_cast<caffe::MemoryDataLayer<float>>( net->layer_by_name("data") );
////    int height = train_input_layer->height();
////    int weight = train_input_layer->width();
////    int channel = train_input_layer->channels();

    const std::string image_file{ "../../../dataset/MNIST/train-images.idx3-ubyte"};
    const std::string train_label_file{ "../../../dataset/MNIST/train-labels.idx1-ubyte" };
    const std::string test_file{ "../../../dataset/MNIST/t10k-images.idx3-ubyte" };
    const std::string test_label_file{ "../../../dataset/MNIST/t10k-labels.idx1-ubyte" };

    Ml::data_converter<float> train;

    train.load_dataset_mnist(image_file,train_label_file);
    Ml::data_converter<float> test;
    test.load_dataset_mnist(test_file,test_label_file);
    auto train_data =train.get_data();
    auto train_label =train.get_label();
    auto test_data = test.get_data();
    auto test_label=test.get_label();





}