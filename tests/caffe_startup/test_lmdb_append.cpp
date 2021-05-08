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
#include "caffe/proto/caffe.pb.h"
#include <caffe/solver.hpp>
#include <caffe/sgd_solvers.hpp>
#include <caffe/layers/memory_data_layer.hpp>
#include <caffe/util/db.hpp>
#include <caffe/util/format.hpp>


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
	// leveldb
	leveldb::DB* db = NULL;
	leveldb::Options options;
	options.error_if_exists = true;
	options.create_if_missing = true;
	options.write_buffer_size = 268435456;
	leveldb::WriteBatch* batch = NULL;
	
	// Open db
	if (db_backend == "leveldb") {  // leveldb
		LOG(INFO) << "Opening leveldb " << db_path;
		leveldb::Status status = leveldb::DB::Open(
				options, db_path, &db);
		CHECK(status.ok()) << "Failed to open leveldb " << db_path
		                   << ". Is it already existing?";
		batch = new leveldb::WriteBatch();
	}
	else if (db_backend == "lmdb") {  // lmdb
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
	else {
		LOG(FATAL) << "Unknown db backend " << db_backend;
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
	
	static int static_counter = 0;
	for (int item_id = 100*static_counter; item_id < 100*(static_counter + 1); ++item_id) {
		image_file.read(pixels, rows * cols);
		label_file.read(&label, 1);
		datum.set_data(pixels, rows*cols);
		datum.set_label(label);
#ifdef __linux__
		int ret = snprintf(key_cstr, kMaxKeyLength, "%08d", item_id);
#else
		int ret = _snprintf(key_cstr, kMaxKeyLength, "%08d", item_id);
#endif
		if (ret == kMaxKeyLength || ret < 0) {
			printf("warning ");
			key_cstr[kMaxKeyLength - 1] = 0;
		}
		datum.SerializeToString(&value);
		std::string keystr(key_cstr);
		
		// Put in db
		if (db_backend == "leveldb") {  // leveldb
			batch->Put(keystr, value);
		}
		else if (db_backend == "lmdb") {  // lmdb
			mdb_data.mv_size = value.size();
			mdb_data.mv_data = reinterpret_cast<void*>(&value[0]);
			mdb_key.mv_size = keystr.size();
			mdb_key.mv_data = reinterpret_cast<void*>(&keystr[0]);
			CHECK_EQ(mdb_put(mdb_txn, mdb_dbi, &mdb_key, &mdb_data, 0), MDB_SUCCESS) << "mdb_put failed";
		}
		else {
			LOG(FATAL) << "Unknown db backend " << db_backend;
		}
		
		if (++count % 100 == 0) {
			// Commit txn
			if (db_backend == "leveldb") {  // leveldb
				db->Write(leveldb::WriteOptions(), batch);
				delete batch;
				batch = new leveldb::WriteBatch();
			}
			else if (db_backend == "lmdb") {  // lmdb
				CHECK_EQ(mdb_txn_commit(mdb_txn), MDB_SUCCESS) << "mdb_txn_commit failed";
				CHECK_EQ(mdb_txn_begin(mdb_env, NULL, 0, &mdb_txn), MDB_SUCCESS) << "mdb_txn_begin failed";
			}
			else {
				LOG(FATAL) << "Unknown db backend " << db_backend;
			}
		}
	}
	static_counter ++;
	delete[] pixels;
}


const std::string argv_test[] {"../../../dataset/MNIST/t10k-images.idx3-ubyte",
                               "../../../dataset/MNIST/t10k-labels.idx1-ubyte",
                               "../../../dataset/MNIST/test-lmdb"};
const std::string argv_train[] { "../../../dataset/MNIST/train-images.idx3-ubyte",
                                 "../../../dataset/MNIST/train-labels.idx1-ubyte",
                                 "../../../dataset/MNIST/train-lmdb" };

// mnist convert to lmdb or leveldb
int mnist_convert()
{
	convert_dataset(argv_train[0].c_str(), argv_train[1].c_str(), argv_train[2].c_str(), "lmdb");
	convert_dataset(argv_test[0].c_str(), argv_test[1].c_str(), argv_test[2].c_str(), "lmdb");
	fprintf(stderr, "mnist convert finish\n");
	return 0;
}


int lenet_5_mnist_train()
{
#ifdef CPU_ONLY
	caffe::Caffe::set_mode(caffe::Caffe::CPU);
#else
	caffe::Caffe::set_mode(caffe::Caffe::GPU);
#endif

#ifdef _MSC_VER
	const std::string filename{ "E:/GitCode/Caffe_Test/test_data/Net/lenet-5_mnist_windows_solver.prototxt" };
#else
	const std::string filename{ "../../../dataset/MNIST/lenet_solver.prototxt" };
#endif
	caffe::SolverParameter solver_param;
	if (!caffe::ReadProtoFromTextFile(filename.c_str(), &solver_param)) {
		fprintf(stderr, "parse solver.prototxt fail\n");
		return -1;
	}
	
	boost::filesystem::remove_all(argv_train[2].c_str());
	boost::filesystem::remove_all(argv_test[2].c_str());
	mnist_convert();
	int max_imum_counter = 0;
	caffe::SGDSolver<float> solver(solver_param);
	
	while (max_imum_counter++ < 10)
	{
		mnist_convert(); // convert MNIST to LMDB
		solver.Step(1);
	}
	
	fprintf(stdout, "train finish\n");
	
	return 0;
}

//This test is expected to fail
int main(int argc, char *argv[])
{
	lenet_5_mnist_train();
	
	return 0;
}
