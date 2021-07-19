#include <iostream>
#include <filesystem>

#include "rocksdb_api.hpp"
#include "boost_serialization_wrapper.hpp"

#include "../block.hpp"

constexpr char output_dir_name[] = "blocks";

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		std::cout << "usage: block_db_to_json {block_db path}" << std::endl;
		return 0;
	}
	
	std::filesystem::path current_path = std::filesystem::current_path();
	std::filesystem::path output_path = current_path / output_dir_name;
	if (!std::filesystem::exists(output_path)) std::filesystem::create_directories(output_path);
	std::filesystem::path db_path = argv[1];
	
	rocksdb::DB* block_database;
	{
		rocksdb::Options options;
		rocksdb::Status status;
		options.create_if_missing = false;
		options.IncreaseParallelism();
		options.OptimizeLevelStyleCompaction();
		
		rocksdb::BlockBasedTableOptions table_options;
		table_options.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10));
		options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_options));
		status = rocksdb::DB::Open(options, db_path.string(), &block_database);
		CHECK(status.ok()) << "failed to open block database";
	}
	
	//read database
	std::map<size_t, std::string> block_container;
	rocksdb::Iterator* it = block_database->NewIterator(rocksdb::ReadOptions());
	for (it->SeekToFirst(); it->Valid(); it->Next())
	{
		size_t height = std::stoi(it->key().ToString());
		block_container[height] = it->value().ToString();
	}
	
	//write block files
	for (auto [height, block_str]: block_container)
	{
		std::filesystem::path output_block_file_path = output_path / (std::to_string(height) + ".json");
		block target_block = deserialize_wrap<boost::archive::binary_iarchive, block>(block_str);
		i_json_serialization::json output_json = target_block.to_json();
		
		std::ofstream file;
		file.open(output_block_file_path.c_str(), std::ios::binary);
		file << output_json.dump(2);
		file.flush();
		file.close();
		
		std::cout << "block height: " << height << std::endl;
	}
	
	block_database->Close();
	delete block_database;
	return 0;
}