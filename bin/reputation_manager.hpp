#pragma once

#include <unordered_map>
#include <string>

#include <rocksdb_api.hpp>

class reputation_manager
{
public:
	static constexpr double default_reputation = 1.0;
	static constexpr char const *DB_CF_TRANSACTIONS = "transactions";
	reputation_manager(const std::string &db_path) : _db_path(db_path)
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
			CHECK(status.ok()) << "[reputation_manager] failed to open rocksdb for reputation_manager";
			rocksdb::ColumnFamilyHandle* cf;
			status = _db->CreateColumnFamily(rocksdb::ColumnFamilyOptions(), DB_CF_TRANSACTIONS, &cf);
			CHECK(status.ok()) << "[reputation_manager] failed to create column family for reputation_manager";
			status = _db->DestroyColumnFamilyHandle(cf);
			CHECK(status.ok()) << "[reputation_manager] failed to destroy column family reputation_manager";
			delete _db;
		}
		
		_column_families.push_back(rocksdb::ColumnFamilyDescriptor(rocksdb::kDefaultColumnFamilyName, rocksdb::ColumnFamilyOptions()));
		_column_families.push_back(rocksdb::ColumnFamilyDescriptor(DB_CF_TRANSACTIONS, rocksdb::ColumnFamilyOptions()));
		status = rocksdb::DB::Open(options, _db_path, _column_families, &_column_family_handles, &_db);
		CHECK(status.ok()) << "[reputation_manager] failed to open rocksdb for data_storage";
	}
	
	~reputation_manager()
	{
		LOG(INFO) << "flush reputation database";
		std_cout::println("flush reputation database");
		_db->FlushWAL(true);
		
		rocksdb::Status status;
//		for (auto& handler: _column_family_handles)
//		{
//			status = _db->DestroyColumnFamilyHandle(handler);
//			LOG_IF(WARNING, !status.ok()) << "failed to drop the column families in database";
//		}
		
//		status = _db->Close();
//		LOG_IF(WARNING, !status.ok()) << "failed to drop the close the reputation manager database";
//		delete _db;
	}
	
	void get_reputation_map(std::unordered_map<std::string, double>& map)
	{
		for (auto&& item: map)
		{
			item.second = get_reputation(item.first);
		}
	}
	
	double get_reputation(const std::string& node_hash)
	{
		std::string reputation_str;
		double reputation;
		rocksdb::Status status = _db->Get(rocksdb::ReadOptions(), node_hash, &reputation_str);
		if (status.ok())
		{
			try
			{
				reputation = std::stold(reputation_str);
			}
			catch (...)
			{
				LOG(WARNING) << "[reputation_manager] failed to load reputation for " << node_hash <<", possibly corrupted db";
				reputation = 0.0;
			}
		}
		else
		{
			reputation = default_reputation;
			rocksdb::WriteOptions write_options;
			write_options.sync = true;
			status = _db->Put(write_options, node_hash, std::to_string(reputation));
			CHECK(status.ok()) << "[reputation_manager] failed to put data in rocksdb for reputation_manager";
		}
		
		return reputation;
	}
	
	void update_reputation(const std::string& node_hash, double reputation)
	{
		rocksdb::WriteOptions write_options;
		write_options.sync = true;
		rocksdb::Status status = _db->Put(write_options, node_hash, std::to_string(reputation));
		CHECK(status.ok()) << "[reputation_manager] failed to put data in rocksdb for reputation_manager";
	}
	
	void update_reputation(const std::unordered_map<std::string, double> reputation_map)
	{
		rocksdb::WriteOptions write_options;
		write_options.sync = true;
		rocksdb::WriteBatch batch;
		for (auto&& item: reputation_map)
		{
			batch.Put(item.first, std::to_string(item.second));
		}
		rocksdb::Status status = _db->Write(write_options, &batch);
		CHECK(status.ok()) << "[reputation_manager] failed to put data in rocksdb for reputation_manager";
	}

private:
	// index 0 for default, 1 for transactions of each node
	std::vector<rocksdb::ColumnFamilyDescriptor> _column_families;
	std::vector<rocksdb::ColumnFamilyHandle*> _column_family_handles;
	
	rocksdb::DB *_db;
	std::string _db_path;
	
};