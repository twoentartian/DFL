#include <string>
#include <iostream>
#include <filesystem>

#include <glog/logging.h>
#include <rocksdb_api.hpp>


void print_help()
{
	std::cout << "print_rocksdb {db_path}" << std::endl;
}

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		print_help();
		return 0;
	}
	std::string db_path = argv[1];
	if (!std::filesystem::exists(db_path))
	{
		std::cout << "database not exist" << std::endl;
		return -1;
	}
	
	std::vector<std::string> column_family_name;
	for (int i = 2; i < argc; ++i)
	{
		column_family_name.emplace_back(argv[i]);
	}
	
	rocksdb::DB* db;
	rocksdb::Options options;
	rocksdb::Status status;
	options.create_if_missing = false;
	options.IncreaseParallelism();
	options.OptimizeLevelStyleCompaction();
	rocksdb::BlockBasedTableOptions table_options;
	table_options.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10));
	options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_options));
	
	std::vector<rocksdb::ColumnFamilyDescriptor> _column_families;
	std::vector<rocksdb::ColumnFamilyHandle*> _column_family_handles;
	_column_families.emplace_back(rocksdb::kDefaultColumnFamilyName, rocksdb::ColumnFamilyOptions());
	for (auto&& cf: column_family_name)
	{
		_column_families.emplace_back(cf, rocksdb::ColumnFamilyOptions());
	}
	
	status = rocksdb::DB::Open(options, db_path, _column_families, &_column_family_handles, &db);
	CHECK(status.ok()) << "failed to open rocksdb for data_storage";
	
	for (int cf_index = 0; cf_index < _column_families.size(); cf_index++)
	{
		std::cout << "--------------    Column Family: " << _column_families[cf_index].name << "    --------------" << std::endl;
		{
			rocksdb::Iterator* it = db->NewIterator(rocksdb::ReadOptions(), _column_family_handles[cf_index]);
			for (it->SeekToFirst(); it->Valid(); it->Next())
			{
				std::cout << it->key().ToString() << ": " << it->value().ToString() << std::endl;
			}
		}
	}
	
	delete db;
	return 0;
}