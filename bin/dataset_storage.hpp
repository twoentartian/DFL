#pragma once

#include <memory>
#include <string>
#include <vector>
#include <random>
#include <unordered_map>
#include <filesystem>
#include <condition_variable>
#include <mutex>

#include <boost/noncopyable.hpp>

#include <glog/logging.h>
#include <rocksdb_api.hpp>
#include <network.hpp>
#include <ml_layer.hpp>
#include <boost_serialization_wrapper.hpp>
#include <util.hpp>

#include "std_output.hpp"

// Database structure
//------------------------------------
// unordered_map{str{label}, counter} // DB_LABELS: counter for the dataset with that label
//------------------------------------
// dataset section
// key : str{label}-{counter}
// value : {dataset}(default) epoch(cf:epoch)
//------------------------------------

template<typename DType>
class dataset_content
{
public:
	Ml::tensor_blob_like<DType> data;
	Ml::tensor_blob_like<DType> label;
	
	template<class Archive>
	void serialize(Archive &ar, const unsigned int version)
	{
		ar & data;
		ar & label;
	}

private:
	friend class boost::serialization::access;
};

template<typename DType>
class dataset_storage : boost::noncopyable
{
public:
	static constexpr char const *DB_LABELS = "labels";
	static constexpr char const *DB_CF_EPOCH = "epoch";
	
	using reserved_sapce_full_callback = std::function<void(const std::vector<Ml::tensor_blob_like<DType>>&data, const std::vector<Ml::tensor_blob_like<DType>>&label)>;
	
	dataset_storage(const std::string &db_path, int reserved_in_memory_size) : _db_path(db_path), _reserved_in_memory_size(reserved_in_memory_size)
	{
		rocksdb::Options options;
		rocksdb::Status status;
		options.create_if_missing = true;
		options.IncreaseParallelism();
		options.OptimizeLevelStyleCompaction();
		
		rocksdb::BlockBasedTableOptions table_options;
		table_options.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10));
		options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_options));
		if (!std::filesystem::exists(_db_path))
		{
			status = rocksdb::DB::Open(options, _db_path, &_db);
			CHECK(status.ok()) << "[dataset_storage] failed to open rocksdb for data_storage";
			rocksdb::ColumnFamilyHandle* cf;
			status = _db->CreateColumnFamily(rocksdb::ColumnFamilyOptions(), DB_CF_EPOCH, &cf);
			CHECK(status.ok()) << "[dataset_storage] failed to create column family for data_storage";
			status = _db->DestroyColumnFamilyHandle(cf);
			CHECK(status.ok()) << "[dataset_storage] failed to destroy column family data_storage";
			delete _db;
		}
		
		_column_families.push_back(rocksdb::ColumnFamilyDescriptor(rocksdb::kDefaultColumnFamilyName, rocksdb::ColumnFamilyOptions()));
		_column_families.push_back(rocksdb::ColumnFamilyDescriptor(DB_CF_EPOCH, rocksdb::ColumnFamilyOptions()));
		status = rocksdb::DB::Open(options, _db_path, _column_families, &_column_family_handles, &_db);
		CHECK(status.ok()) << "[dataset_storage] failed to open rocksdb for data_storage";
		
		init_db();
		
		//update reserved memory space
		_reserved_write_loc = 0;
		auto[data, label] = get_random_data(_reserved_in_memory_size);
		_reserved_data.swap(data);
		_reserved_label.swap(label);
		_reserved_data.resize(_reserved_in_memory_size);
		_reserved_label.resize(_reserved_in_memory_size);
	}
	
	~dataset_storage() noexcept
	{
		stop_network_service();
		
		LOG(INFO) << "flush dataset database";
		std_cout::println("flush dataset database");
		_db->FlushWAL(true);
		
		rocksdb::Status status;
		{
			std::lock_guard guard(_db_lock);
			rocksdb::Status status;
//			for (auto& handler: _column_family_handles)
//			{
//				status = _db->DestroyColumnFamilyHandle(handler);
//				LOG_IF(WARNING, !status.ok()) << "failed to drop the column families in database";
//			}
//			status = _db->Close();
			LOG_IF(WARNING, !status.ok()) << "failed to close the dataset database";
		}
//		delete _db;
	}
	
	void start_network_service(uint16_t port, size_t worker)
	{
		_p2p_server.start_service(port, worker);
		_p2p_server.set_receive_callback([this](const char* data, int size, std::string ip) -> std::string {
			return network_receive_callback(data, size);
		});
		
	}
	
	void stop_network_service()
	{
		_p2p_server.stop_service();
	}
	
	void add_data(const std::vector<Ml::tensor_blob_like<DType>> &data, const std::vector<Ml::tensor_blob_like<DType>> &label)
	{
		//update database
		CHECK(data.size() == label.size()) << "[dataset_storage] data.size() != label.size()";
		rocksdb::WriteBatch batch;
		for (int index = 0; index < label.size(); ++index)
		{
			std::string label_str = label[index].get_str();
			auto label_iter = _counter_by_label.find(label_str);
			std::string key;
			if (label_iter == _counter_by_label.end())
			{
				//new labels
				key = label_str + "-0";
				_counter_by_label[label_str] = 1;
			}
			else
			{
				//old labels
				key = label_str + "-" + std::to_string(label_iter->second);
				label_iter->second++;
			}
			dataset_content<DType> content;
			content.data = data[index];
			content.label = label[index];
			std::string data_str = serialize_wrap<boost::archive::binary_oarchive>(content).str();
			batch.Put(_column_family_handles[0], key, data_str);
			batch.Put(_column_family_handles[1], key, std::to_string(0));
		}
		update_labels_in_db(batch);
		
		rocksdb::WriteOptions write_options;
		write_options.sync = true;
		rocksdb::Status status;
		{
			std::lock_guard guard(_db_lock);
			status = _db->Write(write_options, &batch);
		}
		assert(status.ok());
		
		//update reserved space
		//int start_loc = data.size() > _reserved_in_memory_size ? data.size() - _reserved_in_memory_size : 0;
		for (int i = 0; i < data.size(); ++i)
		{
			insert_data_into_reserved_space(data[i], label[i]);
		}
	}
	
	// return {data, label}
	std::tuple<std::vector<Ml::tensor_blob_like<DType>>, std::vector<Ml::tensor_blob_like<DType>>> get_reserved_data()
	{
		return {_reserved_data, _reserved_label};
	}
	
	// return {data, label}
	// if the current data in DB == 0, the return will be empty vector.
	// if the current data in DB < size(arg), the output will contain duplicated data to ensure the size.
	std::tuple<std::vector<Ml::tensor_blob_like<DType>>, std::vector<Ml::tensor_blob_like<DType>>> get_random_data(int size)
	{
		std::vector<Ml::tensor_blob_like<DType>> data;
		data.reserve(size);
		std::vector<Ml::tensor_blob_like<DType>> label;
		label.reserve(size);
		
		//calculate total size
		std::vector<std::string> label_str;
		label_str.reserve(_counter_by_label.size());
		std::vector<int> label_size;
		label_size.reserve(_counter_by_label.size());
		int total_size = 0;
		for (auto &&pair: _counter_by_label)
		{
			total_size += pair.second;
			label_str.push_back(pair.first);
			label_size.push_back(pair.second);
		}
		
		if (total_size == 0)
			return {data, label};
		
		//generate random index
		std::vector<int> random_index;
		random_index.resize(size);
		std::random_device dev;
		std::mt19937 rng(dev());
		std::uniform_int_distribution<int> distribution(0, total_size - 1);
		for (int i = 0; i < random_index.size(); ++i)
		{
			random_index[i] = distribution(rng);
		}
		std::vector<int> label_start_loc(label_size.size());
		for (int label_index = 0; label_index < label_size.size(); ++label_index)
		{
			label_start_loc[label_index] = 0;
			for (int i = 0; i < label_index; ++i)
			{
				label_start_loc[label_index] += label_size[i];
			}
		}
		
		//insert data
		for (auto &&random_number: random_index)
		{
			std::string value;
			std::string key;
			while(true)
			{
				int label_index = label_start_loc.size() - 1;
				for (int i = 1; i < label_start_loc.size(); ++i)
				{
					if (random_number < label_start_loc[i])
					{
						label_index = i - 1;
						break;
					}
				}
				key = label_str[label_index] + "-" + std::to_string(random_number - label_start_loc[label_index]);
				rocksdb::Status status = _db->Get(rocksdb::ReadOptions(), key, &value);

				if(status.ok())
				{
					break;
				}
				else
				{
					random_number = distribution(rng);
				}
			}

			dataset_content<DType> target;
			try
			{
				target = deserialize_wrap<boost::archive::binary_iarchive, dataset_content<DType>>(value);
			}
			catch (...)
			{
				LOG(FATAL) << "[dataset_storage] unable to deserialize data, key: " << key << ", possibly corrupted db";
			}
			data.push_back(target.data);
			label.push_back(target.label);
		}
		return {data, label};
	}
	
	void set_full_callback(reserved_sapce_full_callback callback)
	{
		_full_reserved_space_callback = callback;
	}
	
private:
	// index 0 for default, 1 for epoch
	std::vector<rocksdb::ColumnFamilyDescriptor> _column_families;
	std::vector<rocksdb::ColumnFamilyHandle*> _column_family_handles;
	
	const std::string _db_path;
	rocksdb::DB *_db;
	std::mutex _db_lock;

	std::unordered_map<std::string, int> _counter_by_label;
	int _reserved_in_memory_size;
	std::vector<Ml::tensor_blob_like<DType>> _reserved_data;
	std::vector<Ml::tensor_blob_like<DType>> _reserved_label;
	int _reserved_write_loc;
	
	reserved_sapce_full_callback _full_reserved_space_callback;
	
	network::p2p _p2p_server;
	
	std::string network_receive_callback(const char* data, int size)
	{
		std::string data_str(data, size);
		std::tuple<std::vector<Ml::tensor_blob_like<DType>>, std::vector<Ml::tensor_blob_like<DType>>> received_dataset;
		try
		{
			received_dataset = deserialize_wrap<boost::archive::binary_iarchive, std::tuple<std::vector<Ml::tensor_blob_like<DType>>, std::vector<Ml::tensor_blob_like<DType>>>>(data_str);
		}
		catch (...)
		{
			LOG(WARNING) << "[dataset_storage] network: received a corrupted dataset";
			return "corrupted data";
		}
		auto& [dataset_data,dataset_label] = received_dataset;
		if (dataset_data.size() != dataset_label.size())
		{
			LOG(WARNING) << "[dataset_storage] network: received a corrupted dataset";
			return "corrupted data";
		}
		add_data(dataset_data, dataset_label);
		return "success";
	}
	
	void update_labels_in_db()
	{
		std::string labels_str = serialize_wrap<boost::archive::binary_oarchive>(_counter_by_label).str();
		rocksdb::Status status;
		{
			std::lock_guard guard(_db_lock);
			status = _db->Put(rocksdb::WriteOptions(), DB_LABELS, labels_str);
		}
		CHECK(status.ok()) << "failed to write db labels";
	}
	
	void update_labels_in_db(rocksdb::WriteBatch &batch)
	{
		std::string labels_str = serialize_wrap<boost::archive::binary_oarchive>(_counter_by_label).str();
		batch.Put(DB_LABELS, labels_str);
	}
	
	void init_db()
	{
		//Init labels in database
		std::string labels_str;
		rocksdb::Status status = _db->Get(rocksdb::ReadOptions(), DB_LABELS, &labels_str);
		if (!status.ok())
		{
			// new db, init labels
			update_labels_in_db();
		}
		else
		{
			// old db
			try
			{
				_counter_by_label = deserialize_wrap<boost::archive::binary_iarchive, std::unordered_map<std::string, int>>(labels_str);
			}
			catch (...)
			{
				LOG(WARNING) << "failed to load data labels, possibly corrupted db";
				
				//TODO: rebuild labels from data
				exit(-1);
			}
		}
	}
	
	void insert_data_into_reserved_space(const Ml::tensor_blob_like<DType> &data, const Ml::tensor_blob_like<DType> &label)
	{
		_reserved_data[_reserved_write_loc] = data;
		_reserved_label[_reserved_write_loc] = label;
		_reserved_write_loc++;
		if (_reserved_write_loc == _reserved_in_memory_size)
		{
			_reserved_write_loc = 0;
			if (_full_reserved_space_callback)
			{
				_full_reserved_space_callback(_reserved_data,_reserved_label);
			}
		}
	}
};
