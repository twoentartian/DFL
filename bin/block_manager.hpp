#pragma once

#include <rocksdb_api.hpp>

#include "block.hpp"
#include "transaction.hpp"

/**
 * (1) initialize block path first
 * (2) set genesis content
 *
 */

class block_manager
{
public:
	block_manager(const std::string &db_path, int seconds_for_receiving_confirmation = 60) : _db_path_block(db_path), _seconds_for_receiving_confirmation(seconds_for_receiving_confirmation)
	{
		_db_path_block.assign(db_path);
		{
			rocksdb::Options options;
			rocksdb::Status status;
			options.create_if_missing = true;
			options.IncreaseParallelism();
			options.OptimizeLevelStyleCompaction();
			
			rocksdb::BlockBasedTableOptions table_options;
			table_options.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10));
			options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_options));
			status = rocksdb::DB::Open(options, _db_path_block.string(), &_db_blocks);
			CHECK(status.ok()) << "[transaction_storage_for_block] failed to open rocksdb for _db_blocks";
		}
		
		set_height_from_db();
	}
	
	~block_manager()
	{
		while (_current_generated_block)
		{
			std_cout::println("a block is currently under generating, hash:" + _current_generated_block->final_hash + ", please wait until it finishes");
			std::this_thread::sleep_for(std::chrono::seconds(5));
		}
		
		LOG(INFO) << "flush block manager database";
		std_cout::println("flush block manager database");
		_db_blocks->FlushWAL(true);
	}
	
	void set_genesis_content(const std::string genesis_content)
	{
		block genesis_block;
		genesis_block.content.creator.node_pubkey = "";
		genesis_block.content.creator.node_address = "";
		genesis_block.content.transaction_container.clear();
		genesis_block.content.block_generated_time = 0;
		genesis_block.content.memo = "";
		genesis_block.content.previous_block_hash = "";
		genesis_block.content.genesis_block_hash = "";
		genesis_block.content.genesis_content = genesis_content;
		
		genesis_block.block_content_hash = crypto::sha256_digest(genesis_block.content).getTextStr_lowercase();
		genesis_block.height = 0;
		genesis_block.block_confirmation_container.clear();
		genesis_block.block_finalization_time = 0;
		
		auto hash = crypto::sha256_digest(genesis_block);
		_genesis_hash = hash.getTextStr_lowercase();
		genesis_block.final_hash = _genesis_hash;
		
		if (_height == 0)
		{
			genesis_block.final_hash = _genesis_hash;
			store_block(genesis_block);
			_previous_block_hash = _genesis_hash;
		}
		else
		{
			std::string genesis_content_db;
			auto status = _db_blocks->Get(rocksdb::ReadOptions(), "0", &genesis_content_db);
			LOG_IF(ERROR, !status.ok()) << "[block] failed to access block database";
			block genesis_block_db = deserialize_wrap<boost::archive::binary_iarchive, block>(genesis_content_db);
			if (genesis_block_db.final_hash != _genesis_hash)
			{
				LOG(ERROR) << "[block] genesis block mismatch";
				return;
			}
			load_previous_hash_from_db();
		}
	}
	
	std::optional<block> generate_block(const std::vector<transaction>& transactions)
	{
		std::lock_guard guard(_block_lock);
		
		//current block not finished?
		if (_current_generated_block) return std::nullopt;
		
		_current_generated_block.reset(new block());
		if (_genesis_hash.empty()) LOG(ERROR) << "[block] genesis hash not set";

		_current_generated_block->height = _height;
		_current_generated_block->content.creator.node_address = global_var::address.getTextStr_lowercase();
		_current_generated_block->content.creator.node_pubkey = global_var::public_key.getTextStr_lowercase();
		_current_generated_block->content.genesis_block_hash = _genesis_hash;
		_current_generated_block->content.previous_block_hash = _previous_block_hash;
		_current_generated_block->content.block_generated_time = time_util::get_current_utc_time();
		for (auto& trans: transactions)
		{
			_current_generated_block->content.transaction_container[trans.hash_sha256] = trans;
		}
		_current_generated_block->content.creator = transactions[0].content.creator;
		
		auto hash_hex = crypto::sha256_digest(_current_generated_block->content);
		_current_generated_block->block_content_hash = hash_hex.getTextStr_lowercase();
		
		return {*_current_generated_block};
	}
	
	std::tuple<bool, std::string> append_block_confirmation(const block_confirmation& confirm)
	{
		std::lock_guard guard(_block_lock);
		
		if (!_current_generated_block) return {false, "block confirmation windows expires"};
		
		//check confirmation
		if (confirm.block_hash != _current_generated_block->block_content_hash)
		{
			return {false, "block hash mismatch"};
		}
		
		auto iter_transaction = _current_generated_block->content.transaction_container.find(confirm.transaction_hash);
		if (iter_transaction == _current_generated_block->content.transaction_container.end())
		{
			return {false, "transaction not found"};
		}
		
		auto iter_receipt = iter_transaction->second.receipts.find(confirm.receipt_hash);
		if (iter_receipt == iter_transaction->second.receipts.end())
		{
			return {false, "transaction receipt not found"};
		}
		
		if (iter_receipt->second.content.creator != confirm.creator )
		{
			return {false, "confirmation and receipt creator mismatch"};
		}
		
		//verify hash & signature
		auto confirm_hash_hex = crypto::sha256_digest(confirm);
		if (confirm_hash_hex.getTextStr_lowercase() != confirm.final_hash)
		{
			return {false, "hash verification fail"};
		}
		crypto::hex_data pubkey_hex(confirm.creator.node_pubkey);
		crypto::hex_data signature_hex(confirm.signature);
		if (crypto::ecdsa_openssl::verify(signature_hex, confirm_hash_hex, pubkey_hex) != true)
		{
			return {false, "signature verification fail"};
		}
		
		_current_generated_block->block_confirmation_container[confirm.final_hash] = confirm;
		
		return {true,""};
	}
	
	/**
	 * Store the block in cache to database
	 */
	block store_finalized_block()
	{
		std::lock_guard guard(_block_lock);
		
		//attach the finalized information
		_current_generated_block->block_finalization_time = time_util::get_current_utc_time();
		auto final_hash_hex = crypto::sha256_digest(*_current_generated_block);
		_current_generated_block->final_hash = final_hash_hex.getTextStr_lowercase();
		
		std::string block_content_db = serialize_wrap<boost::archive::binary_oarchive>(*_current_generated_block).str();
		block output = *_current_generated_block;
		auto status = _db_blocks->Put(rocksdb::WriteOptions(), std::to_string(_height), block_content_db);
		LOG_IF(ERROR, !status.ok()) << "[block] failed to access block database";
		_height++;
		
		_previous_block_hash = _current_generated_block->final_hash;
		
		//clear current block
		_current_generated_block.reset();
		
		return std::move(output);
	}
	
private:
	std::filesystem::path  _db_path_block;
	rocksdb::DB* _db_blocks;
	uint64_t _height;
	std::string _genesis_hash;
	std::string _previous_block_hash;
	int _seconds_for_receiving_confirmation;
	std::shared_ptr<block> _current_generated_block;
	std::mutex _block_lock;
	
	/**
	 * Store certain block to database
	 */
	void store_block(const block& blk)
	{
		std::string block_content_db = serialize_wrap<boost::archive::binary_oarchive>(blk).str();
		auto status = _db_blocks->Put(rocksdb::WriteOptions(), std::to_string(_height), block_content_db);
		LOG_IF(ERROR, !status.ok()) << "[block] failed to access block database";
		_height++;
		
		_previous_block_hash = blk.final_hash;
	}
	
	/**
	 * Find current height from the database
	 */
	void set_height_from_db()
	{
		//check height
		_height = 0;
		bool empty = true;
		std::string value;
		rocksdb::Iterator* it = _db_blocks->NewIterator(rocksdb::ReadOptions());
		for (it->SeekToFirst(); it->Valid(); it->Next())
		{
			empty = false;
			uint64_t target_height = std::stoi(it->key().ToString());
			if (target_height > _height) _height = target_height;
		}
		if (!empty) _height++;
	}
	
	/**
	 * Find the latest block and load the previous hash
	 */
	void load_previous_hash_from_db()
	{
		std::string last_content_db;
		auto status = _db_blocks->Get(rocksdb::ReadOptions(), std::to_string(_height - 1), &last_content_db);
		block last_block_db = deserialize_wrap<boost::archive::binary_iarchive, block>(last_content_db);
		_previous_block_hash = last_block_db.final_hash;
	}
};
