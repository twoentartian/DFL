//
// Created by tyd.
//

#define CPU_ONLY

#include <iostream>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include <stdint.h>
#include <sys/stat.h>
#include <fstream>
#include <random>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <google/protobuf/text_format.h>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <lmdb.h>
#include <boost/algorithm/string.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/scoped_ptr.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <caffe/caffe.hpp>
#include <caffe/util/io.hpp>
#include <caffe/blob.hpp>
#include <caffe/common.hpp>
#include <caffe/layer_factory.hpp>
#include <caffe/util/math_functions.hpp>
#include <caffe/syncedmem.hpp>
#include <caffe/util/upgrade_proto.hpp>
#include <caffe/layers/pooling_layer.hpp>
#include <caffe/net.hpp>
#include <caffe/proto/caffe.pb.h>
#include <caffe/solver.hpp>
#include <caffe/sgd_solvers.hpp>
#include <caffe/layers/memory_data_layer.hpp>
#include <caffe/util/db.hpp>
#include <caffe/util/format.hpp>

#include <ml_layer.hpp>
#include <data_convert.hpp>

#include <util.hpp>

// random data
// merge model: average
int main(int argc, char *argv[])
{
	google::InitGoogleLogging(argv[0]);
	
	constexpr int NUMBER_NODES = 5;
	constexpr int MAX_ITER = 1000;
	constexpr int TRAIN_BATCH_SIZE = 64; //this must a multiple of batch_size, it will be divided into many smaller batch automatically
	constexpr int TEST_ITER = 100;
	constexpr int TEST_BATCH_SIZE = 100;
	
	const std::string solver_path = "../../../dataset/MNIST/lenet_solver_memory.prototxt";
	
	const std::string train_dataset_path = "../../../dataset/MNIST/train-images.idx3-ubyte";
	const std::string train_label_dataset_path = "../../../dataset/MNIST/train-labels.idx1-ubyte";
	
	const std::string test_dataset_path = "../../../dataset/MNIST/t10k-images.idx3-ubyte";
	const std::string test_label_dataset_path = "../../../dataset/MNIST/t10k-labels.idx1-ubyte";
	
	Ml::data_converter<float> train_dataset;
	train_dataset.load_dataset_mnist(train_dataset_path,train_label_dataset_path);
	Ml::data_converter<float> test_dataset;
	test_dataset.load_dataset_mnist(test_dataset_path,test_label_dataset_path);
	
	Ml::MlCaffeModel<float, caffe::SGDSolver> models[NUMBER_NODES];
	Ml::caffe_parameter_net<float> parameters[NUMBER_NODES];
	for(int i = 0; i < NUMBER_NODES; i++)
	{
		models[i].load_caffe_model(solver_path);
	}
	
	int iter = 0;
	while (iter < MAX_ITER)
	{
		for (int node_index = 0; node_index < NUMBER_NODES; ++node_index)
		{
			Ml::tensor_blob_like<float> label;
			label.getShape() = {1};
			label.getData() = {float (node_index*2)};
			auto [train_data_0, train_label_0] = train_dataset.get_random_data_by_Label(label, TRAIN_BATCH_SIZE/2);
			label.getData() = {float (node_index*2+1)};
			auto [train_data_1, train_label_1] = train_dataset.get_random_data_by_Label(label, TRAIN_BATCH_SIZE/2);
			std::vector<Ml::tensor_blob_like<float>> train_data = util::vector_concatenate(train_data_0, train_data_1);
			std::vector<Ml::tensor_blob_like<float>> train_label = util::vector_concatenate(train_label_0, train_label_1);
			util::vector_twin_shuffle(train_data,train_label);
			
			models[node_index].train(train_data,train_label);
			
			//test
			if (iter % TEST_ITER == 0)
			{
				auto [test_data, test_label] = test_dataset.get_random_data(TEST_BATCH_SIZE);
				float accuracy = models[node_index].evaluation(test_data, test_label);
				std::cout << boost::format("iter: %1% node:%2% accuracy: %3%") % models[node_index].get_iter() % node_index % accuracy << std::endl;
			}
			
			//average
			parameters[node_index] = models[node_index].get_parameter();
			for (int apply_index = 0; apply_index < NUMBER_NODES; ++apply_index)
			{
				if (apply_index == node_index)
				{
					continue;
				}
				
				parameters[apply_index] = models[apply_index].get_parameter();
				for (int layer_index = 0; layer_index < parameters[node_index].getLayers().size(); ++layer_index)
				{
					if (parameters[node_index].getLayers()[layer_index].getBlob_p()->empty())
					{
						continue;
					}
					auto& data1 = parameters[node_index].getLayers()[layer_index].getBlob_p()->getData();
					auto& data2 = parameters[apply_index].getLayers()[layer_index].getBlob_p()->getData();
					for (int data_index = 0; data_index < data1.size(); ++data_index)
					{
						data2[data_index] = (data1[data_index] + data2[data_index])/2;
					}
					models[apply_index].set_parameter(parameters[apply_index]);
				}
			}
		}
		
		iter ++;
	}
	
	
	
}
