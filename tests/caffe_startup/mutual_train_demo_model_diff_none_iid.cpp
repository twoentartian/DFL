#define CPU_ONLY

#include <iostream>
#include <cstring>
#include <map>
#include <string>
#include <random>
#include <glog/logging.h>
#include <boost/format.hpp>
#include <ml_layer.hpp>

//return <max, min>
template<typename T>
std::tuple<T, T> find_max_min(T* data, size_t size)
{
	T max, min;
	max=*data;min=*data;
	for (int i = 0; i < size; ++i)
	{
		if (data[i] > max) max = data[i];
		if (data[i] < min) min = data[i];
	}
	return {max,min};
}

int main(int argc, char *argv[])
{
	google::InitGoogleLogging(argv[0]);
	
	constexpr int NUMBER_NODES = 4;
	constexpr int MAX_ITER = 120;
	constexpr int TRAIN_BATCH_SIZE = 64; //this must a multiple of batch_size, it will be divided into many smaller batch automatically
	constexpr int TEST_ITER = 20;
	constexpr int TEST_BATCH_SIZE = 100;
	constexpr float filter_limit = 0.5; // 0 to drop all data, 1 to stop dropping
	constexpr bool record_layer_to_file = false;
	
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
	for(int i = 0; i < NUMBER_NODES; i++)
	{
		models[i].load_caffe_model(solver_path);
	}
	
	int iter = 0;
	Ml::fed_avg_buffer<Ml::caffe_parameter_net<float>> parameter_buffer(NUMBER_NODES);
	while (iter < MAX_ITER)
	{
		bool exit = false;
		for (int node_index = 0; node_index < NUMBER_NODES; ++node_index)
		{
			Ml::tensor_blob_like<float> label;
			label.getShape() = {1};
			//label.getData() = {float (0)};
			//auto [train_data, train_label] = train_dataset.get_random_data_by_Label(label, TRAIN_BATCH_SIZE);
			//auto [train_data, train_label] = train_dataset.get_random_data(TRAIN_BATCH_SIZE);
			
//			//extreme non iid situation
//			label.getData() = {float (node_index*2)};
//			auto [train_data_0, train_label_0] = train_dataset.get_random_data_by_Label(label, TRAIN_BATCH_SIZE/2);
//			label.getData() = {float (node_index*2+1)};
//			auto [train_data_1, train_label_1] = train_dataset.get_random_data_by_Label(label, TRAIN_BATCH_SIZE/2);
//			std::vector<Ml::tensor_blob_like<float>> train_data = util::vector_concatenate(train_data_0, train_data_1);
//			std::vector<Ml::tensor_blob_like<float>> train_label = util::vector_concatenate(train_label_0, train_label_1);
//			util::vector_twin_shuffle(train_data,train_label);
			
			std::random_device dev;
			std::mt19937 rng(dev());
			std::uniform_real_distribution<float> distribution(0.1,0.3);
			std::uniform_real_distribution<float> dense_distribution(0.8,1.2);
			Ml::non_iid_distribution<float> non_iid_distribution;
			for (int i = 0; i < 10; ++i)
			{
				label.getData() = {float (i)};
				if(i % NUMBER_NODES == node_index)
				{
					non_iid_distribution.add_distribution(label, dense_distribution(rng));
				}
				else
				{
					non_iid_distribution.add_distribution(label, distribution(rng));
				}
				
			}
			
			auto [train_data, train_label] = train_dataset.get_random_non_iid_dataset(non_iid_distribution, TRAIN_BATCH_SIZE);
			
			auto parameter_before = models[node_index].get_parameter();
			models[node_index].train(train_data,train_label);
			auto parameter_after = models[node_index].get_parameter();
			auto parameter_diff = parameter_after - parameter_before;
			
			auto& layers = parameter_diff.getLayers();
			
			//drop models
			for (int i = 0; i < layers.size(); ++i)
			{
				auto& blob = layers[i].getBlob_p();
				auto& blob_data = blob->getData();
				if (blob_data.empty())
				{
					continue;
				}
				auto [max, min] = find_max_min(blob_data.data(), blob_data.size());
				auto range = max - min;
				auto lower_bound = min + range * filter_limit * 0.5;
				auto higher_bound = max - range * filter_limit * 0.5;
				
				int drop_count = 0;
				for (auto&& data: blob_data)
				{
					if(data < higher_bound && data > lower_bound)
					{
						//drop
						drop_count++;
						data = NAN;
					}
				}
				
				std::cout << "node:" << node_index << "    iter:" << iter << "    layer:" << i << "    drop:" << ((float)drop_count)/blob_data.size() << "(" << drop_count <<"/" << blob_data.size() <<")" << std::endl;
			}
			
			auto parameter_filtered = parameter_diff.dot_divide(parameter_diff).dot_product(parameter_after);
			
			parameter_buffer.add(parameter_filtered);
			
			//update all nodes' model
			auto model_average = parameter_buffer.average_ignore();
			for (int node_index_update = 0; node_index_update < NUMBER_NODES; ++node_index_update)
			{
				if (node_index_update == node_index)
				{
					//self
					parameter_before.patch_weight(model_average);
					models[node_index_update].set_parameter(parameter_before);
				}
			}
			
			
			//output layers
			if constexpr(record_layer_to_file)
			{
				for (int i = 0; i < layers.size(); ++i)
				{
					auto blob = layers[i].getBlob_p();
					std::ofstream output_file("node" + std::to_string(node_index) + "blob" + std::to_string(i) + "iter" + std::to_string(iter), std::ios::binary);
					
					auto blob_data = blob->getData();
					for (int data_index = 0; data_index < blob_data.size(); ++data_index)
					{
						output_file << std::to_string(data_index) << " " << std::to_string(blob_data[data_index]) << std::endl;
					}
					output_file.flush();
					output_file.close();
				}
			}
			
			
			//test
			if (iter % TEST_ITER == 0)
			{
				auto [test_data_random, test_label_random] = test_dataset.get_random_data(TEST_BATCH_SIZE);
				float accuracy = models[node_index].evaluation(test_data_random, test_label_random);

				std::cout << boost::format("iter: %1% node:%2% accuracy: %3%") % iter % node_index % accuracy << std::endl;
				if (accuracy > 0.995) exit = true;
			}
			
		}
		
		iter ++;
		if (exit) break;
	}
	
}