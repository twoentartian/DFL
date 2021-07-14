#define BOOST_TEST_MAIN

#include <filesystem>
#include <boost/test/included/unit_test.hpp>
#include <ml_layer.hpp>

BOOST_AUTO_TEST_SUITE (caffe_boost_unit_test)
	
	BOOST_AUTO_TEST_CASE (model_compress)
	{
		google::InitGoogleLogging(std::filesystem::current_path().c_str());
		
		constexpr int TRAIN_BATCH_SIZE = 64;
		const std::string train_dataset_path = "../../../dataset/MNIST/train-images.idx3-ubyte";
		const std::string train_label_dataset_path = "../../../dataset/MNIST/train-labels.idx1-ubyte";
		const std::string solver_path = "../../../dataset/MNIST/lenet_solver_memory.prototxt";
		
		Ml::data_converter<float> train_dataset;
		train_dataset.load_dataset_mnist(train_dataset_path,train_label_dataset_path);
		
		Ml::MlCaffeModel<float, caffe::SGDSolver> model;
		model.load_caffe_model(solver_path);
		
		auto [train_data, train_label] = train_dataset.get_random_data(TRAIN_BATCH_SIZE);
		auto parameter_before = model.get_parameter();
		model.train(train_data,train_label);
		auto parameter_after = model.get_parameter();
		
		size_t drop = 0, total = 0;
		auto raw_diff_model = Ml::model_compress::compress_by_diff_get_model(parameter_before, parameter_after, 0.5);
		auto str_data = Ml::model_compress::compress_by_diff(parameter_before, parameter_after, 0.5, &total, &drop);
		
		auto decompress_diff_model = Ml::model_compress::decompress_by_diff<float>(str_data);
	}
	
	BOOST_AUTO_TEST_CASE (model_stream)
	{
		google::InitGoogleLogging(std::filesystem::current_path().c_str());
		
		constexpr int TRAIN_BATCH_SIZE = 64;
		const std::string train_dataset_path = "../../../dataset/MNIST/train-images.idx3-ubyte";
		const std::string train_label_dataset_path = "../../../dataset/MNIST/train-labels.idx1-ubyte";
		const std::string solver_path = "../../../dataset/MNIST/lenet_solver_memory.prototxt";
		
		Ml::data_converter<float> train_dataset;
		train_dataset.load_dataset_mnist(train_dataset_path,train_label_dataset_path);
		
		Ml::MlCaffeModel<float, caffe::SGDSolver> model;
		model.load_caffe_model(solver_path);
		
		auto [train_data, train_label] = train_dataset.get_random_data(TRAIN_BATCH_SIZE);
		auto parameter_before = model.get_parameter();
		model.train(train_data,train_label);
		auto parameter_after = model.get_parameter();
		
		auto data_compressed = Ml::model_interpreter<float>::generate_model_stream_by_compress_diff(parameter_before, parameter_after, 0.5);
		auto [diff_model, diff_type] = l::model_interpreter<float>::parse_model_stream(data_compressed);
		
		auto data_normal = l::model_interpreter<float>::generate_model_stream_normal(parameter_after);
		auto [normal_model, normal_type] = l::model_interpreter<float>::parse_model_stream(data_normal);
		
		std::cout << "compressed size:" << data_compressed.size() << "  normal size:" << data_normal.size() << std::endl;
		
		BOOST_CHECK(normal_type == interpreter.normal);
		BOOST_CHECK(diff_type == interpreter.compressed_by_diff);
	}
	
	BOOST_AUTO_TEST_CASE (lz4_compress)
	{
		std::string data_raw = util::get_random_str(500000);
		int max_compressed_size = LZ4::Compress_CalculateDstSize(data_raw.size());
		char* compressed_output = new char[max_compressed_size];
		int real_compressed_size = LZ4::Compress(data_raw.data(), data_raw.size(), compressed_output, max_compressed_size);
		
		std::string output(compressed_output, real_compressed_size);
		int decompress_size = LZ4::Decompress_CalculateDstSize(output.data());
		char* decompress_content = new char[decompress_size];
		int real_decompressed_size = LZ4::Decompress(output.data(), output.size(), decompress_content);
		BOOST_CHECK(real_decompressed_size == decompress_size);
		std::string ss_str(decompress_content, decompress_size);
		BOOST_CHECK(data_raw == ss_str);
		
		std::ofstream output1,output2;
		output1.open("output1.hex", std::ios::binary);
		output1 << data_raw;
		output1.flush();
		output1.close();
		
		output2.open("output2.hex", std::ios::binary);
		output2 << ss_str;
		output2.flush();
		output2.close();
	}
	
BOOST_AUTO_TEST_SUITE_END()

