//
// Created by tyd.
//

#include <lmdb.h>

#include <caffe/util/io.hpp>
#include <caffe/sgd_solvers.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>

#include <ml_layer/ml_abs.hpp>
#include <ml_layer/caffe.hpp>
#include <ml_layer/tensor_blob_like.hpp>
#include <ml_layer/fed_avg_buffer.hpp>

BOOST_AUTO_TEST_SUITE (ml_caffe_test)

BOOST_AUTO_TEST_CASE (structure_same_test)
{
	Ml::MlCaffeModel<float,caffe::SGDSolver> model1;
	model1.load_caffe_model("../../../dataset/MNIST/lenet_solver.prototxt");
	Ml::MlCaffeModel<float,caffe::SGDSolver> model2 = model1;
	
	
	Ml::caffe_parameter_net<float> net1 = model1.get_parameter();
	for (int i = 0; i < net1.getLayers().size(); ++i)
	{
		auto& data = net1.getLayers()[i].getBlob_p().get()->getData();
		for (auto&& s : data)
		{
			s = 0;
		}
	}
	model1.set_parameter(net1);
	std::string structure1 = model1.get_network_structure_info();
	std::string structure2 = model2.get_network_structure_info();

	BOOST_CHECK(structure1 == structure2);
}

BOOST_AUTO_TEST_CASE (get_model_parameter)
{
	Ml::MlCaffeModel<float,caffe::SGDSolver> model1;
	model1.load_caffe_model("../../../dataset/MNIST/lenet_solver.prototxt");
	Ml::MlCaffeModel<float,caffe::SGDSolver> model2 = model1;
	Ml::caffe_parameter_net<float> parameter1 = model1.get_parameter();
	for (int i = 0; i < parameter1.getLayers().size(); ++i)
	{
		auto& data = parameter1.getLayers()[i].getBlob_p()->getData();
		for (auto&& s : data)
		{
			s = 0;
		}
	}
	model1.set_parameter(parameter1);
	auto parameter2 = model1.get_parameter();
	bool pass = true;
	for (int i = 0; i < parameter1.getLayers().size(); ++i)
	{
		auto& data1 = parameter1.getLayers()[i].getBlob_p()->getData();
		auto& data2 = parameter2.getLayers()[i].getBlob_p()->getData();
		if(data1 != data2) pass = false;
		break;
	}
	BOOST_CHECK(pass);
}

BOOST_AUTO_TEST_CASE (serialization)
{
	Ml::MlCaffeModel<float, caffe::SGDSolver> model1;
	model1.load_caffe_model("../../../dataset/MNIST/lenet_solver.prototxt");
	Ml::MlCaffeModel<float, caffe::SGDSolver> model2;
	model2.load_caffe_model("../../../dataset/MNIST/lenet_solver.prototxt");
	
	Ml::caffe_parameter_net<float> parameter1 = model1.get_parameter();
	for (int i = 0; i < parameter1.getLayers().size(); ++i)
	{
		auto& data = parameter1.getLayers()[i].getBlob_p()->getData();
		for (auto&& s : data)
		{
			s = 0;
		}
	}
	model1.set_parameter(parameter1);
	
	auto ss = model1.serialization(Ml::model_serialization_boost_binary);
	model2.deserialization(Ml::model_serialization_boost_binary,ss);
	auto parameter2 = model2.get_parameter();
	bool pass = true;
	for (int i = 0; i < parameter1.getLayers().size(); ++i)
	{
		auto& data1 = parameter1.getLayers()[i].getBlob_p()->getData();
		auto& data2 = parameter2.getLayers()[i].getBlob_p()->getData();
		if(data1 != data2) pass = false;
		break;
	}
	BOOST_CHECK(pass);
}

BOOST_AUTO_TEST_CASE (net_parameter_add_divide)
{
	Ml::MlCaffeModel<float, caffe::SGDSolver> model1;
	model1.load_caffe_model("../../../dataset/MNIST/lenet_solver.prototxt");
	Ml::caffe_parameter_net<float> parameter = model1.get_parameter();
	auto parameterx2 = parameter + parameter;
	auto parameterx3 = parameter + parameterx2;
	auto parameterx4 = parameterx2 + parameterx2;
	auto parameter1 = parameterx2 / 2;
	auto parameter2 = parameterx3 / 3;
	auto parameter3 = parameterx4 / 4;
	BOOST_CHECK(parameter == parameter1);
	//BOOST_CHECK(parameter == parameter2);
	BOOST_CHECK(parameter == parameter3);
}

BOOST_AUTO_TEST_CASE (fed_avg_buffer)
{
	Ml::MlCaffeModel<float, caffe::SGDSolver> model1;
	model1.load_caffe_model("../../../dataset/MNIST/lenet_solver.prototxt");
	Ml::caffe_parameter_net<float> parameter = model1.get_parameter();
	
	Ml::fed_avg_buffer<Ml::caffe_parameter_net<float>, 6> parameter_buffer;
	parameter_buffer.write(parameter);
	parameter_buffer.write(parameter);
	auto parameter0 = parameter_buffer.average();
	parameter_buffer.write(parameter);
	parameter_buffer.write(parameter);
	auto parameter1 = parameter_buffer.average();
	parameter_buffer.write(parameter);
	parameter_buffer.write(parameter);
	parameter_buffer.write(parameter);
	parameter_buffer.write(parameter);
	auto parameter2 = parameter_buffer.average();
	
	BOOST_CHECK(parameter == parameter0);
	BOOST_CHECK(parameter == parameter1);
	BOOST_CHECK(parameter != parameter2);
}

static uint32_t swap_endian(uint32_t val)
{
	val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
	return (val << 16) | (val >> 16);
}

BOOST_AUTO_TEST_CASE (caffe_ext_dtype)
{
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
	
	LOG(INFO) << "A total of " << num_items << " items.";
	LOG(INFO) << "Rows: " << rows << " Cols: " << cols;
	
	//std::vector<caffe::Datum> datums;
	//datums.resize(num_items);
	constexpr int TRAIN_SIZE = 3200;
	constexpr int TEST_SIZE = 100;
	std::vector<Ml::tensor_blob_like<float>> data_blob,label_blob,test_data_blob,test_label_blob;
	data_blob.resize(TRAIN_SIZE);
	label_blob.resize(data_blob.size());
	test_data_blob.resize(TEST_SIZE);
	test_label_blob.resize(test_data_blob.size());
	
	//prepare train data
	{
		char label;
		auto* pixels = new char[rows * cols];
		for (int item_id = 0; item_id < data_blob.size(); ++item_id)
		{
			auto& current_data_blob = data_blob[item_id].getData();
			auto& current_label_blob = label_blob[item_id].getData();
			data_blob[item_id].getShape() = {1, static_cast<int>(rows), static_cast<int>(cols)};
			label_blob[item_id].getShape() = {1};
			
			image_file.read(pixels, rows * cols);
			label_file.read(&label, 1);
			current_data_blob.resize(rows * cols);
			for (int i = 0; i < rows * cols; ++i)
			{
				current_data_blob[i] = float(uint8_t(pixels[i]));
			}
			current_label_blob.resize(1);
			current_label_blob[0] = label;
		}
		delete[] pixels;
	}
	
	//prepare test data
	{
		char label;
		auto* pixels = new char[rows * cols];
		for (int item_id = 0; item_id < test_data_blob.size(); ++item_id)
		{
			auto& current_data_blob = test_data_blob[item_id].getData();
			auto& current_label_blob = test_label_blob[item_id].getData();
			test_data_blob[item_id].getShape() = {1, static_cast<int>(rows), static_cast<int>(cols)};
			test_label_blob[item_id].getShape() = {1};
			
			image_file.read(pixels, rows * cols);
			label_file.read(&label, 1);
			current_data_blob.resize(rows * cols);
			for (int i = 0; i < rows * cols; ++i)
			{
				current_data_blob[i] = float(uint8_t(pixels[i]));
			}
			current_label_blob.resize(1);
			current_label_blob[0] = label;
		}
		delete[] pixels;
	}
	
	Ml::caffe_solver_ext<float, caffe::SGDSolver> model1("../../../dataset/MNIST/lenet_solver_memory.prototxt");
	BOOST_CHECK(model1.checkValidFirstLayer_memoryLayer());
	
	for (int repeat = 0; repeat < 3; ++repeat)
	{
		model1.TrainDataset(data_blob, label_blob);
		auto results = model1.TestDataset(test_data_blob, test_label_blob);
		for (auto&& result : results)
		{
			auto& [accuracy,loss] = result;
			std::cout << "accuracy:" << accuracy << "    loss:" << loss << std::endl;
		}
	}

}

BOOST_AUTO_TEST_SUITE_END( )
