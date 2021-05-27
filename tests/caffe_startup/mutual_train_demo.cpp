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
#include <caffe/proto/caffe.pb.h>
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

static void convert_dataset(const char* image_filename, const char* label_filename, const char* db_path, const std::string& db_backend)
{
	// Open files
	std::ifstream image_file(image_filename, std::ios::in | std::ios::binary);
	std::ifstream label_file(label_filename, std::ios::in | std::ios::binary);
	CHECK(image_file) << "Unable to open file " << image_filename;
	CHECK(label_file) << "Unable to open file " << label_filename;
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
	
	// lmdb
	MDB_env *mdb_env = NULL;
	MDB_dbi mdb_dbi;
	MDB_val mdb_key, mdb_data;
	MDB_txn *mdb_txn = NULL;
	
	// Open db
	{
		int rc;
		LOG(INFO) << "Opening lmdb " << db_path;
		boost::filesystem::create_directory(db_path);
		
		CHECK_EQ(mdb_env_create(&mdb_env), MDB_SUCCESS) << "mdb_env_create failed";
		//CHECK_EQ(mdb_env_set_mapsize(mdb_env, 1099511627776), MDB_SUCCESS) << "mdb_env_set_mapsize failed";//1TB
		CHECK_EQ(mdb_env_set_mapsize(mdb_env, 107374182), MDB_SUCCESS) << "mdb_env_set_mapsize failed";//100MB
		CHECK_EQ(mdb_env_open(mdb_env, db_path, 0, 0664), MDB_SUCCESS) << "mdb_env_open failed";
		CHECK_EQ(mdb_txn_begin(mdb_env, NULL, 0, &mdb_txn), MDB_SUCCESS) << "mdb_txn_begin failed";
		CHECK_EQ(mdb_open(mdb_txn, NULL, 0, &mdb_dbi), MDB_SUCCESS) << "mdb_open failed. Does the lmdb already exist? ";
	}
	
	// Storing to db
	char label;
	char* pixels = new char[rows * cols];
	int count = 0;
	const int kMaxKeyLength = 10;
	char key_cstr[kMaxKeyLength];
	std::string value;
	
	caffe::Datum datum;
	datum.set_channels(1);
	datum.set_height(rows);
	datum.set_width(cols);
	LOG(INFO) << "A total of " << num_items << " items.";
	LOG(INFO) << "Rows: " << rows << " Cols: " << cols;
	
	for (int item_id = 0; item_id < num_items; ++item_id) {
		image_file.read(pixels, rows * cols);
		label_file.read(&label, 1);
		datum.set_data(pixels, rows*cols);
		datum.set_label(label);
		int ret = snprintf(key_cstr, kMaxKeyLength, "%08d", item_id);
		if (ret == kMaxKeyLength || ret < 0) {
			printf("warning ");
			key_cstr[kMaxKeyLength - 1] = 0;
		}
		datum.SerializeToString(&value);
		std::string keystr(key_cstr);
		
		// Put in db
		mdb_data.mv_size = value.size();
		mdb_data.mv_data = reinterpret_cast<void*>(&value[0]);
		mdb_key.mv_size = keystr.size();
		mdb_key.mv_data = reinterpret_cast<void*>(&keystr[0]);
		CHECK_EQ(mdb_put(mdb_txn, mdb_dbi, &mdb_key, &mdb_data, 0), MDB_SUCCESS) << "mdb_put failed";
		
		if (++count % 1000 == 0) {
			CHECK_EQ(mdb_txn_commit(mdb_txn), MDB_SUCCESS) << "mdb_txn_commit failed";
			CHECK_EQ(mdb_txn_begin(mdb_env, NULL, 0, &mdb_txn), MDB_SUCCESS) << "mdb_txn_begin failed";
		}
	}
	// write the last batch
	if (count % 1000 != 0) {
		CHECK_EQ(mdb_txn_commit(mdb_txn), MDB_SUCCESS) << "mdb_txn_commit failed";
		mdb_close(mdb_env, mdb_dbi);
		mdb_env_close(mdb_env);
		LOG(ERROR) << "Processed " << count << " files.";
	}
	delete[] pixels;
}

// mnist convert to lmdb or leveldb
int mnist_convert()
{
	const std::string argv_test[] {"../../../dataset/MNIST/t10k-images.idx3-ubyte",
	                               "../../../dataset/MNIST/t10k-labels.idx1-ubyte",
	                               "../../../dataset/MNIST/test-lmdb"};
	const std::string argv_train[] { "../../../dataset/MNIST/train-images.idx3-ubyte",
	                                 "../../../dataset/MNIST/train-labels.idx1-ubyte",
	                                 "../../../dataset/MNIST/train-lmdb" };
	convert_dataset(argv_train[0].c_str(), argv_train[1].c_str(), argv_train[2].c_str(), "lmdb");
	convert_dataset(argv_test[0].c_str(), argv_test[1].c_str(), argv_test[2].c_str(), "lmdb");
	fprintf(stderr, "mnist convert finish\n");
	return 0;
}


int lenet_5_mnist_train()
{
	caffe::Caffe::set_mode(caffe::Caffe::CPU);
	
	const std::string filename{ "../../../dataset/MNIST/lenet_solver.prototxt" };
	
	caffe::SolverParameter solver_param;
	if (!caffe::ReadProtoFromTextFile(filename.c_str(), &solver_param)) {
		fprintf(stderr, "parse solver.prototxt fail\n");
		return -1;
	}
	
	mnist_convert(); // convert MNIST to LMDB

	constexpr int MODEL_COUNT = 4;
	constexpr int iteration = 300;
	
	Ml::MlCaffeModel<float, caffe::SGDSolver> models[MODEL_COUNT];
	Ml::caffe_parameter_net<float> parameters[MODEL_COUNT];
	for (int i = 0; i < MODEL_COUNT; ++i)
	{
		models[i].load_caffe_model(filename);
	}
	
	for (int i = 0; i < MODEL_COUNT; ++i)
	{
		parameters[i] = models[i].get_parameter();
	}
	
	int counter = 0;
	while (counter++ < iteration)
	{
		for (int train_index = 0; train_index < MODEL_COUNT; ++train_index)
		{
			models[train_index].raw()->Step(1);
			parameters[train_index] = models[train_index].get_parameter();
			
			//average
			for (int apply_index = 0; apply_index < MODEL_COUNT; ++apply_index)
			{
				if (apply_index == train_index)
				{
					continue;
				}
				
				for (int layer_index = 0; layer_index < parameters[train_index].getLayers().size(); ++layer_index)
				{
					if (parameters[train_index].getLayers()[layer_index].getBlob_p()->empty())
					{
						continue;
					}
					auto& data1 = parameters[train_index].getLayers()[layer_index].getBlob_p()->getData();
					auto& data2 = parameters[apply_index].getLayers()[layer_index].getBlob_p()->getData();
					for (int data_index = 0; data_index < data1.size(); ++data_index)
					{
						data2[data_index] = (data1[data_index] + data2[data_index])/2;
					}
					models[apply_index].set_parameter(parameters[apply_index]);
				}
			}
		}
		
	}
	
	fprintf(stdout, "train finish\n");
	
	return 0;
}

int main(int argc, char *argv[])
{
	lenet_5_mnist_train();
	
	return 0;
}
