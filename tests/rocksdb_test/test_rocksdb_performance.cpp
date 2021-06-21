#include <cassert>
#include <string>
#include <iostream>
#include <chrono>

#include <rocksdb_api.hpp>
#include <auto_multi_thread.hpp>

#define TEST_FREQUENCY	(100000)

char* randomstr()
{
	static char buf[1024];
	int len = rand() % 768 + 255;
	for (int i = 0; i < len; ++i) {
		buf[i] = 'A' + rand() % 26;
	}
	buf[len] = '\0';
	return buf;
}

int main()
{
	{
		rocksdb::DB* db;
		rocksdb::Options options;
		options.create_if_missing = true;
		options.IncreaseParallelism();
		options.OptimizeLevelStyleCompaction();
		
		// open database
		rocksdb::BlockBasedTableOptions table_options;
		table_options.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10));
		options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_options));
		
		rocksdb::Status status = rocksdb::DB::Open(options, "./testdb", &db);
		assert(status.ok());
		
		srand(2017);
		std::string* k = new std::string[TEST_FREQUENCY];
		for (int i = 0; i < TEST_FREQUENCY; ++i)
		{
			k[i] = (randomstr());
		}
		std::string v("0123456789");
		v.append(v).append(v).append(v).append(v).append(v);
		
		// test append
		{
			auto start = std::chrono::system_clock::now();
			rocksdb::WriteOptions write_options;
			write_options.sync = true;
			rocksdb::WriteBatch batch;
			for (int i = 0; i < TEST_FREQUENCY; ++i)
			{
				batch.Put(k[i], v);
			}
			status = db->Write(write_options, &batch);
			assert(status.ok());
			auto end = std::chrono::system_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
			
			std::cout << TEST_FREQUENCY << " append costs: "
			          << double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den
			          << "s" << std::endl;
		}
		// test get
		{
			auto start = std::chrono::system_clock::now();
			std::string* v2 = new std::string[TEST_FREQUENCY];
			for (int i = 0; i < TEST_FREQUENCY; ++i)
			{
				status = db->Get(rocksdb::ReadOptions(), k[i], &v2[i]);
				assert(status.ok());
			}
			auto end = std::chrono::system_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
			
			std::cout << TEST_FREQUENCY << " get costs: "
			          << double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den
			          << "s" << std::endl;
			// verify data
			std::string ss;
			for (int i = 0; i < TEST_FREQUENCY; ++i) {
				if (v2[i] != v) {
					std::cout << "index " << i << " result is wrong" << std::endl;
					std::cout << v2[i] << std::endl;
				}
			}
			delete[] v2;
		}
		
		// test modification
		{
			auto start = std::chrono::system_clock::now();
			v.append(v);
			for (int i = 0; i < TEST_FREQUENCY; ++i)
			{
				status = db->Put(rocksdb::WriteOptions(), k[i], v);
				assert(status.ok());
			}
			auto end = std::chrono::system_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
			
			std::cout << TEST_FREQUENCY << " modification costs: "
			          << double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den
			          << "s" << std::endl;
		}
		
		// test delete
		{
			auto start = std::chrono::system_clock::now();
			for (int i = 0; i < TEST_FREQUENCY; ++i)
			{
				status = db->Delete(rocksdb::WriteOptions(), k[i]);
				assert(status.ok());
			}
			auto end = std::chrono::system_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
			
			std::cout << TEST_FREQUENCY << " deletion costs: "
			          << double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den
			          << "s" << std::endl;
		}
		delete[] k;
		delete db;
	}
	
	return 0;
}