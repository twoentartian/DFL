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
#include "caffe/proto/caffe.pb.h"
#include <caffe/solver.hpp>
#include <caffe/sgd_solvers.hpp>
#include <caffe/layers/memory_data_layer.hpp>
#include <caffe/util/db.hpp>
#include <caffe/util/format.hpp>

#include <ml_layer.hpp>

static uint32_t swap_endian(uint32_t val)
{
	val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
	return (val << 16) | (val >> 16);
}

std::vector<caffe::Datum> GetDatum(const std::string& data_path, const std::string& label_path)
{
	std::ifstream image_file(data_path, std::ios::in | std::ios::binary);
	std::ifstream label_file(label_path, std::ios::in | std::ios::binary);
	CHECK(image_file) << "Unable to open file " << data_path;
	CHECK(label_file) << "Unable to open file " << label_path;
	
	uint32_t magic;
	uint32_t num_items;
	uint32_t num_labels;
	uint32_t rows;
	uint32_t cols;
	
	image_file.read(reinterpret_cast<char*>(&magic), 4);
	magic = swap_endian(magic);
	CHECK_EQ(magic, 2051) << "Incorrect image file magic.";
	label_file.read(reinterpret_cast<char*>(&magic), 4);
	magic = swap_endian(magic);
	CHECK_EQ(magic, 2049) << "Incorrect label file magic.";
	image_file.read(reinterpret_cast<char*>(&num_items), 4);
	num_items = swap_endian(num_items);
	label_file.read(reinterpret_cast<char*>(&num_labels), 4);
	num_labels = swap_endian(num_labels);
	CHECK_EQ(num_items, num_labels);
	image_file.read(reinterpret_cast<char*>(&rows), 4);
	rows = swap_endian(rows);
	image_file.read(reinterpret_cast<char*>(&cols), 4);
	cols = swap_endian(cols);
	
	//Caffe Datum
	LOG(INFO) << "A total of " << num_items << " items.";
	LOG(INFO) << "Rows: " << rows << " Cols: " << cols;
	
	std::vector<caffe::Datum> datums;
	datums.resize(num_items);
	char label;
	char* pixels = new char[rows * cols];
	for (int item_id = 0; item_id < num_items; ++item_id)
	{
		datums[item_id].set_channels(1);
		datums[item_id].set_height(rows);
		datums[item_id].set_width(cols);
		
		image_file.read(pixels, rows * cols);
		label_file.read(&label, 1);
		datums[item_id].set_data(pixels, rows * cols);
		datums[item_id].set_label(label);
	}
	delete[] pixels;
	
	return datums;
}

int main(int argc, char *argv[])
{
	const std::string solver_path = "../../../dataset/MNIST/lenet_solver_memory.prototxt";
	
	const std::string train_dataset_path = "../../../dataset/MNIST/train-images.idx3-ubyte";
	const std::string train_label_dataset_path = "../../../dataset/MNIST/train-labels.idx1-ubyte";
	
	const std::string test_dataset_path = "../../../dataset/MNIST/t10k-images.idx3-ubyte";
	const std::string test_label_dataset_path = "../../../dataset/MNIST/t10k-labels.idx1-ubyte";

	auto train_data = GetDatum(train_dataset_path, train_label_dataset_path);
	auto test_data = GetDatum(test_dataset_path, test_label_dataset_path);
	
	constexpr int BATCH_SIZE = 64;
	constexpr int TEST_BATCH_SIZE = 100;
	
	constexpr int MODEL_COUNT = 2;
	constexpr int iteration = 300;
	
	Ml::MlCaffeModel<float> models[MODEL_COUNT];
	Ml::caffe_parameter_net<float> parameters[MODEL_COUNT];
	for (int i = 0; i < MODEL_COUNT; ++i)
	{
		models[i].load_caffe_model<caffe::SGDSolver>(solver_path);
	}
	
	for (int i = 0; i < MODEL_COUNT; ++i)
	{
		parameters[i] = models[i].get_parameter();
	}
	
	std::vector<caffe::Datum> data[MODEL_COUNT];
	for (int index = 0; index < MODEL_COUNT; ++index)
	{
		auto first_layer = models[index].raw().get()->net().get()->layers()[0];
		auto* memory_data_layer = static_cast<caffe::MemoryDataLayer<float>*>(first_layer.get());
		auto test_first_layer = models[index].raw().get()->test_nets().data()->get()->layers()[0];
		auto* test_memory_data_layer = static_cast<caffe::MemoryDataLayer<float>*>(test_first_layer.get());
		
		//test
		test_memory_data_layer->AddDatumVector(test_data);
		
		//train
		std::random_device rd;
		std::mt19937 mt(rd());
		std::uniform_int_distribution<int> dist(0, train_data.size());

		data[index].resize(BATCH_SIZE*iteration);

		for (int i = 0; i < BATCH_SIZE*iteration; ++i)
		{
//			auto &selection = train_data[dist(mt)];
//			data[index].push_back(selection);
			data[index][i] = train_data[dist(mt)];
		}
		
		memory_data_layer->AddDatumVector(data[index]);
		
//		std::cout << "123" << std::endl;
//		for (auto&& target : data)
//		{
//			if (target.channels() != 1)
//			{
//				std::system("pause");
//			}
//		}
	}
	
	int counter = 0;
	while (counter < iteration)
	{
//		//set data
//		for (int model_index = 0; model_index < MODEL_COUNT; ++model_index)
//		{
//			auto first_layer = models[model_index].raw().get()->net().get()->layers()[0];
//			auto* memory_data_layer = static_cast<caffe::MemoryDataLayer<float>*>(first_layer.get());
//			auto test_first_layer = models[model_index].raw().get()->test_nets().data()->get()->layers()[0];
//			auto* test_memory_data_layer = static_cast<caffe::MemoryDataLayer<float>*>(test_first_layer.get());
//
//			//random
//			std::random_device rd;
//			std::mt19937 mt(rd());
//			std::uniform_int_distribution<int> dist(0, train_data.size());
//			{
//				int single_cell_data_size = train_data[0].height() * train_data[0].width();
////				float* data_f = new float[BATCH_SIZE * single_cell_data_size];
////				float* label_f = new float[BATCH_SIZE];
//
//				std::vector<caffe::Datum> data;
//				data.reserve(BATCH_SIZE);
//				for (int i = 0; i < BATCH_SIZE; ++i)
//				{
//					auto& selection = train_data[dist(mt)];
//
//					//datum
//					data.push_back(selection);
//
////					//Reset
////					for (int j = 0; j < single_cell_data_size; ++j)
////					{
////						data_f[j + i * single_cell_data_size] = float(static_cast<uint8_t>(selection.data()[i])) / 256.0f;
////					}
////					label_f[i] = selection.label();
//				}
//				//memory_data_layer->AddDatumVector(data);
////				memory_data_layer->Reset(data_f,label_f,BATCH_SIZE);
////				delete[] data_f;
////				delete[] label_f;
//			}
////			std::uniform_int_distribution<int> dist2(0, test_data.size());
////			if (counter % 1 == 0)
////			{
////				std::vector<caffe::Datum> data;
////				data.reserve(TEST_BATCH_SIZE);
////				for (int i = 0; i < TEST_BATCH_SIZE; ++i)
////				{
////					data.push_back(test_data[dist2(mt)]);
////				}
////				LOG(INFO) << "Add test data at counter: " << counter << " size: " << data.size();
////				test_memory_data_layer->AddDatumVector(data);
////			}
//		}

		
		
		//train
		for (int train_index = 0; train_index < MODEL_COUNT; ++train_index)
		{
			models[train_index].raw()->Step(1);
			parameters[train_index] = models[train_index].get_parameter();
			
//			//average
//			for (int apply_index = 0; apply_index < MODEL_COUNT; ++apply_index)
//			{
//				parameters[apply_index] = models[apply_index].get_parameter();
//				if (apply_index == train_index)
//				{
//					continue;
//				}
//
//				for (int layer_index = 0; layer_index < parameters[train_index].getLayers().size(); ++layer_index)
//				{
//					if (parameters[train_index].getLayers()[layer_index].getBlob_p()->empty())
//					{
//						continue;
//					}
//					auto& data1 = parameters[train_index].getLayers()[layer_index].getBlob_p()->getData();
//					auto& data2 = parameters[apply_index].getLayers()[layer_index].getBlob_p()->getData();
//					for (int data_index = 0; data_index < data1.size(); ++data_index)
//					{
//						data2[data_index] = (data1[data_index] + data2[data_index])/2;
//					}
//					models[apply_index].set_parameter(parameters[apply_index]);
//				}
//			}
		}
		counter++;
	}
	
	fprintf(stdout, "train finish\n");
	return 0;
}
