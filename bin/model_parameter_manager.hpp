//
// Created by jxzhang on 25-07-21.
//

#ifndef DFL_MODEL_PARAMETER_MANAGER_HPP
#define DFL_MODEL_PARAMETER_MANAGER_HPP



#include <unordered_map>
#include <optional>
#include <string>
#include <filesystem>
#include <ctime>
#include <algorithm>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <rocksdb_api.hpp>
#include <ml_layer.hpp>
#include "transaction.hpp"
#include  "../lib/crypto.hpp"
#include "../lib/ml_layer.hpp"
#include "time_util.hpp"


enum model_parameter_storage_hash_mode
{
    model_parameter_storage_MD5=0,
    model_parameter_storage_SHA256,
};

class model_parameter_manager {

public:
    static constexpr char const *DB_CF_MODEL_PARAMETERS = "local model parameters";

    model_parameter_manager(const std::string &model_db_path) : _model_db_path(model_db_path) {
        rocksdb::Options options;
        rocksdb::Status status;

        options.create_if_missing = true;
        options.IncreaseParallelism();

        rocksdb::BlockBasedTableOptions table_options;
        table_options.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10));

        options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_options));

        if (!std::filesystem::exists(_model_db_path)) {
            status = rocksdb::DB::Open(options, model_db_path, &_model_db);
            CHECK(status.ok()) << "[model_parameter_manager] failed to open rocksdb for model_parameter_manager";
            rocksdb::ColumnFamilyHandle *cf;
            status = _model_db->CreateColumnFamily(rocksdb::ColumnFamilyOptions(), DB_CF_MODEL_PARAMETERS,&cf);
            CHECK(status.ok()) << "[model_parameter_manager] failed to create column family for model parameter";
            status = _model_db->DestroyColumnFamilyHandle(cf);
            CHECK(status.ok()) << "[model_parameter_manager] failed to destroy column family for model parameter";
            delete _model_db;
        }

        _model_column_families.push_back(
                rocksdb::ColumnFamilyDescriptor(rocksdb::kDefaultColumnFamilyName, rocksdb::ColumnFamilyOptions()));
        _model_column_families.push_back(
                rocksdb::ColumnFamilyDescriptor(DB_CF_MODEL_PARAMETERS, rocksdb::ColumnFamilyOptions()));

        status = rocksdb::DB::Open(options, _model_db_path, _model_column_families, &_model_column_family_handles,
                                   &_model_db);
        CHECK(status.ok()) << "[model_parameter_manager] failed to open rocksdb for data_storage";

    }

    ~model_parameter_manager()
    {
        for (auto handle : _model_column_family_handles)
        {
            rocksdb::Status status=_model_db->DestroyColumnFamilyHandle(handle);
            CHECK(status.ok()) <<  "[model_parameter_manager] failed to close columnfamily's handles";
        }
        rocksdb::Status status=_model_db->Close();
        CHECK(status.ok())<<"[model_parameter_manager] failed to close database";
        delete _model_db;
    }

    void reset_model_parameter_database()
    {
        rocksdb::Iterator* it=_model_db->NewIterator(rocksdb::ReadOptions());

        std::string temp_key;

        rocksdb::WriteOptions write_options;
        rocksdb::WriteBatch batch;
        write_options.sync = true;

        for (it->SeekToFirst();it->Valid();it->Next())
        {
            temp_key=it->key().ToString();
            batch.Delete(temp_key);
        }

        rocksdb::Status status=_model_db->Write(write_options,&batch);
        CHECK(status.ok()) << "[model_parameter_manager] failed to reset database in rocksdb";

    }

    void reset_model_parameter_database(const std::string &node_hash)
    {
        rocksdb::Iterator* it=_model_db->NewIterator(rocksdb::ReadOptions());

        std::string temp_key;

        rocksdb::WriteOptions write_options;
        rocksdb::WriteBatch batch;
        write_options.sync = true;

        for (it->SeekToFirst();it->Valid();it->Next())
        {
            temp_key=it->key().ToString();
            std::size_t found=temp_key.find(node_hash);
            if (found==0) {
                batch.Delete(temp_key);
            }
        }

        rocksdb::Status status=_model_db->Write(write_options,&batch);
        CHECK(status.ok()) << "[model_parameter_manager] failed to reset database in rocksdb";

    }


    template<typename model_datatype>
    std::optional<std::vector<std::pair<time_t,Ml::caffe_parameter_net<model_datatype>>>> get_model_parameter_net_history(const std::string &node_hash)
    {
        std::vector<std::pair<time_t,Ml::caffe_parameter_net<model_datatype>>> output;

        rocksdb::Iterator* it=_model_db->NewIterator(rocksdb::ReadOptions());
        std::string temp_key;

        for (it->SeekToFirst();it->Valid();it->Next())
        {
            temp_key=it->key().ToString();
            std::size_t found=temp_key.find(node_hash);
            if (found==0 && temp_key!=node_hash)
            {
                auto pair= get_model_parameter_net_from_value<model_datatype>(it->value().ToString());
                if (pair)
                {
                    output.push_back(pair.value());
                }
            }
        }

        if (output.empty())
        {
            LOG(INFO)<<"there is no recording for node: "<<node_hash<<"in the database";
            return std::nullopt;
        }
        else
        {
            std::sort(output.begin(),output.end(), timestamp_comparator<Ml::caffe_parameter_net<model_datatype>>);
            return output;
        }
    }

    template<typename model_datatype>
    std::optional<Ml::caffe_parameter_net<model_datatype>> get_model_parameter_net_newest(const std::string& node_hash)
    {
        std::string parameter_str;

//        string to string divided to stringstream to caffe_parameter_net
        std::string newest_key=node_hash;
        rocksdb::Status status=_model_db->Get(rocksdb::ReadOptions(),newest_key,&parameter_str);
        if (status.IsNotFound())
        {
            LOG(INFO)<<"there is no recording for node: "<<node_hash<<"in the database";
            return std::nullopt;
        }
        else if (!status.ok())
        {
            LOG(WARNING) << "[model_parameter_manager] failed to read data in rocksdb";
            return std::nullopt;
        }
        else
        {
            auto pair=get_model_parameter_net_from_value<model_datatype>(parameter_str);
            if (pair)
            {
                return pair.value().second;
            }
            else
            {
                return std::nullopt;
            }

        }
    }


    template<typename model_datatype>
    void update_model_parameter_net(const std::string &node_hash, const Ml::caffe_parameter_net<model_datatype> model_parameter_net,enum model_parameter_storage_hash_mode mode=model_parameter_storage_MD5)
    {

//        caffe_parameter_net to stringstream to string
        std::ostringstream  output;
        boost::archive::text_oarchive o_archive(output);
        o_archive<<model_parameter_net;

//        use hash to get the second part of the key
        std::string temp_key= key_generator(output.str());
        if (temp_key.empty())
        {
            return;
        }
        rocksdb::WriteOptions write_options;
        rocksdb::WriteBatch batch;
        write_options.sync = true;

        //        add timestamp (in UTC time)

        time_t now=time_util::get_current_utc_time();
        std::string timestamp=time_util::time_to_text(now);

        std::string key=node_hash+"_"+temp_key;
        std::string value=timestamp+"_"+output.str();

        batch.Put(key,value);
        batch.Delete(node_hash);
        batch.Put(node_hash,value);

        rocksdb::Status status=_model_db->Write(write_options,&batch);

        CHECK(status.ok()) << "[model_parameter_manager] failed to put data in rocksdb";

    }

    template<typename model_datatype>
    void update_model_parameter_net(std::shared_ptr<std::vector<transaction>> transactions)
    {
        rocksdb::WriteOptions write_options;
        write_options.sync = true;

        for (int i=0; i< transactions->size();i++)
        {
            std::string node_hash=(*transactions)[i].content.creator.node_address;
            auto temp_model_parameter_net=Ml::model_interpreter<model_datatype>::parse_model_stream((*transactions)[i].content.model_data);
            update_model_parameter_net(node_hash,temp_model_parameter_net);
        }
    }

private:

//    index 0 for default, index 1 is for itself, index 2 is for others
    std::vector<rocksdb::ColumnFamilyDescriptor> _model_column_families;
    std::vector<rocksdb::ColumnFamilyHandle*> _model_column_family_handles;

    rocksdb::DB *_model_db;
    std::string _model_db_path;

    std::string key_generator(const std::string &initial_string, enum model_parameter_storage_hash_mode mode=model_parameter_storage_MD5)
    {
        //        use hash to get part of the key
        std::string temp_key;
        if (mode==model_parameter_storage_MD5)
        {
            crypto::md5 md5_manager;
            auto temp_output=md5_manager.digest(initial_string);
            temp_key=temp_output.getTextStr_lowercase();
        }
        else if(mode==model_parameter_storage_SHA256)
        {
            crypto::sha256 sha256_manager;
            auto temp_output=sha256_manager.digest(initial_string);
            temp_key=temp_output.getTextStr_lowercase();
        }
        else
        {
            std::cout<<"[model_parameter_manager] unknown hash type"<<std::endl;
            temp_key="";
        }
        return temp_key;
    }

    template<typename model_datatype>
    std::optional<std::pair<time_t,Ml::caffe_parameter_net<model_datatype>>> get_model_parameter_net_from_value(const std::string &value)
    {
        Ml::caffe_parameter_net<model_datatype> net_parameter;

//        string slicing
        std::size_t found=value.find("_");

        if (found==std::string::npos)
        {
            LOG(WARNING)<<"there is a corrupted value field";
            return std::nullopt;
        }
        else
        {
//        get timestamp
            time_t timestamp=time_util::text_to_time(value.substr(0,found));

//        get  model parameter
            std::string parameter_str=value.substr(found+1,value.size()-found-1);
            std::stringstream parameter_stream;
            parameter_stream.str(parameter_str);
            boost::archive::text_iarchive i_archive(parameter_stream);
            i_archive >> net_parameter;

            std::pair<time_t, Ml::caffe_parameter_net<model_datatype>> output(timestamp,net_parameter);

            return output;
        }


    }

    template<class class_type>
    static bool timestamp_comparator(const std::pair<time_t,class_type>& pair_1, const std::pair<time_t,class_type>& pair_2)
    {
        double difference= difftime(pair_1.first,pair_2.first);
        return (difference<0);
    }


};

#endif //DFL_MODEL_PARAMETER_MANAGER_HPP
