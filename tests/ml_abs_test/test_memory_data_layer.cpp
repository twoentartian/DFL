//
// Created by tyd.
//

#include <caffe/util/io.hpp>
#include <ml_layer/ml_abs.hpp>
#include <ml_layer/caffe.hpp>
#include <caffe/sgd_solvers.hpp>
#include <caffe/layers/memory_data_layer.hpp>

static uint32_t swap_endian(uint32_t val)
{
	val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
	return (val << 16) | (val >> 16);
}

int main()
{
	Ml::MlCaffeModel<float> model1;
	model1.load_caffe_model<caffe::SGDSolver>("../../../dataset/MNIST/lenet_solver_memory.prototxt");
	
	const std::string train_dataset_path = "../../../dataset/MNIST/train-images.idx3-ubyte";
	const std::string label_dataset_path = "../../../dataset/MNIST/train-labels.idx1-ubyte";

// Open files
	std::ifstream image_file(train_dataset_path, std::ios::in | std::ios::binary);
	std::ifstream label_file(label_dataset_path, std::ios::in | std::ios::binary);
	CHECK(image_file) << "Unable to open file " << train_dataset_path;
	CHECK(label_file) << "Unable to open file " << label_dataset_path;
// Read the magic and the meta data
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
	
	auto first_layer = model1.raw().get()->net().get()->layers()[0];
	auto* memory_data_layer = static_cast<caffe::MemoryDataLayer<float>*>(first_layer.get());
	auto test_first_layer = model1.raw().get()->test_nets().data()->get()->layers()[0];
	auto* test_memory_data_layer = static_cast<caffe::MemoryDataLayer<float>*>(test_first_layer.get());
	
	constexpr int batch_size = 64;
	constexpr int batch_count = 100;
	
	//Train
	int single_cell_data_size = datums[0].height() * datums[0].width();

	std::vector<float> data_f;
	std::vector<float> label_f;
	data_f.reserve(single_cell_data_size * batch_size);
	label_f.reserve(batch_size);
	for (int batch_counter = 0; batch_counter < batch_count; ++batch_counter)
	{
		for (int loc_in_batch = 0; loc_in_batch < batch_size; ++loc_in_batch)
		{
			for (int i = 0; i < single_cell_data_size; ++i)
			{
				float value = float(static_cast<uint8_t>(datums[loc_in_batch + batch_size * batch_counter].data()[i])) / 256.0f;
				data_f.push_back(value);
			}
			label_f.push_back(datums[loc_in_batch + batch_size * batch_counter].label());
		}
	}

//	memory_data_layer->Reset(data_f.data(), label_f.data(), batch_size * batch_count);
//	test_memory_data_layer->Reset(data_f.data(), label_f.data(), batch_size * batch_count);
	
	for (int batch_counter = 0; batch_counter < batch_count; ++batch_counter)
	{
		memory_data_layer->Reset(data_f.data(), label_f.data(), batch_size);
		test_memory_data_layer->Reset(data_f.data(), label_f.data(), 100);
		model1.raw()->Step(1);
	}

	
	std::cout << "train finished" << std::endl;
	
}
