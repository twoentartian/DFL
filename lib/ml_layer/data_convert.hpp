//
// Created by gzr on 01-05-21.
//
#pragma once
#include <sstream>
#include <vector>
#include <memory>
#include <fstream>
#include <random>
#include <unordered_map>
#include <glog/logging.h>

#include <boost_serialization_wrapper.hpp>
#include "tensor_blob_like.hpp"
#include "exception.hpp"
#include "util.hpp"

namespace Ml{
	template <typename DType>
	class non_iid_distribution
	{
	public:
		void add_distribution(const tensor_blob_like<DType>& label, float weight)
		{
			std::string label_str = serialize_wrap<boost::archive::binary_oarchive>(label).str();
			_distribution[label_str] = weight;
		}
		
		const std::unordered_map<std::string, float>& get() const
		{
			return _distribution;
		}
		
	private:
		std::unordered_map<std::string, float> _distribution;
	};
	
    template <typename DType>
    class data_converter
    {
    public:
        void load_dataset_mnist(const std::string &image_filename, const std::string &label_filename) {
            std::ifstream data_file(image_filename, std::ios::in | std::ios::binary);
            std::ifstream label_file(label_filename, std::ios::in | std::ios::binary);
            CHECK(data_file) << "Unable to open file " << image_filename;
            CHECK(label_file) << "Unable to open file " << label_filename;

            uint32_t magic;
            uint32_t num_items;
            uint32_t num_labels;
            uint32_t rows;
            uint32_t cols;
            data_file.read(reinterpret_cast<char *>(&magic), 4);
            magic = swap_endian(magic);
            CHECK_EQ(magic, 2051) << "Incorrect image file magic.";
            label_file.read(reinterpret_cast<char *>(&magic), 4);
            magic = swap_endian(magic);
            CHECK_EQ(magic, 2049) << "Incorrect label file magic.";
            data_file.read(reinterpret_cast<char *>(&num_items), 4);
            num_items = swap_endian(num_items);

            label_file.read(reinterpret_cast<char *>(&num_labels), 4);
            num_labels = swap_endian(num_labels);
            CHECK_EQ(num_items, num_labels);
            data_file.read(reinterpret_cast<char *>(&rows), 4);
            rows = swap_endian(rows);
            data_file.read(reinterpret_cast<char *>(&cols), 4);
            cols = swap_endian(cols);
            
	        _data.resize(num_items);
	        _label.resize(num_items);
	        
	        char temp_label;
	        auto* temp_pixels = new char[rows * cols];
	        for (int item_id = 0; item_id < num_items; ++item_id)
	        {
		       auto& current_data_blob = _data[item_id].getData();
		       auto& current_label_blob = _label[item_id].getData();
		       _data[item_id].getShape() = {1, static_cast<int>(rows), static_cast<int>(cols)};
		       _label[item_id].getShape() = {1};
		
		       data_file.read(temp_pixels, rows * cols);
		       label_file.read(&temp_label, 1);
		       current_data_blob.resize(rows * cols);
		       for (int i = 0; i < rows * cols; ++i)
		       {
			       current_data_blob[i] = float(uint8_t(temp_pixels[i]));
		       }
		       current_label_blob.resize(1);
		       current_label_blob[0] = temp_label;
	        }
	        delete[] temp_pixels;
	        
	        //generate _container_by_label
	        for (int index = 0; index < num_items; ++index)
	        {
	        	std::string key = _label[index].get_str();
	        	auto key_iter = _container_by_label.find(key);
		        if (key_iter == _container_by_label.end())
		        {
			        _container_by_label[key].reserve(num_items/5);
		        }
		        _container_by_label[key].push_back(_data[index]);
	        }
	        for(auto&& iter: _container_by_label)
	        {
		        iter.second.shrink_to_fit();
	        }
        }

        const std::vector<tensor_blob_like<DType>>& get_data() const
        {
            return _data;
        }

        const std::vector<tensor_blob_like<DType>>& get_label() const
        {
            return _label;
        }

        //return: <data,label>
        std::tuple<std::vector<tensor_blob_like<DType>>, std::vector<tensor_blob_like<DType>>> get_random_data(int size)
        {
	        return _get_random_data(size, _data, _label);
        }
	
	    //return: <data,label>
	    std::tuple<std::vector<tensor_blob_like<DType>>, std::vector<tensor_blob_like<DType>>> get_random_data_by_Label(const tensor_blob_like<DType>& arg_label, int size)
	    {
        	//does not exist key
        	const std::string key_str = arg_label.get_str();
			auto iter = _container_by_label.find(key_str);
			if(iter == _container_by_label.end())
			{
				return {{},{}};
			}
			
		    std::vector<tensor_blob_like<DType>> data,label;
		    data.resize(size);label.resize(size);
		    for (int i = 0; i < size; ++i)
		    {
			    std::random_device dev;
			    std::mt19937 rng(dev());
			    std::uniform_int_distribution<int> distribution(0, iter->second.size()-1);
			    int dice = distribution(rng);
			    data[i] = iter->second[dice];
			    label[i] = arg_label;
		    }
		    return {data,label};
	    }
	
	    //please ensure the dataset is larger than the size*100 to ensure the best randomness.
	    //return: <data,label>
	    std::tuple<std::vector<tensor_blob_like<DType>>, std::vector<tensor_blob_like<DType>>> get_random_non_iid_dataset(const non_iid_distribution<DType>& distribution, int size, int enlargement_factor = 100)
	    {
        	float total_weight = 0.0;
        	auto& distribution_map = distribution.get();
		    for (auto iter = distribution_map.begin(); iter != distribution_map.end() ; iter++)
		    {
			    total_weight += iter->second;
		    }
		    std::vector<tensor_blob_like<DType>> data_pool, label_pool;
		    for (auto iter = distribution_map.begin(); iter != distribution_map.end() ; iter++)
		    {
		    	tensor_blob_like<DType> label_blob;
			    label_blob = deserialize_wrap<boost::archive::binary_iarchive, tensor_blob_like<DType>>(iter->first);
			    auto [data,label] = get_random_data_by_Label(label_blob, iter->second / total_weight * size * enlargement_factor);
			    data_pool.insert(data_pool.end(), data.begin(), data.end());
			    label_pool.insert(label_pool.end(), label.begin(), label.end());
		    }
		    return _get_random_data(size, data_pool, label_pool);
	    }
	
	    std::tuple<const std::vector<tensor_blob_like<DType>>&, const std::vector<tensor_blob_like<DType>>& > get_whole_dataset()
	    {
		    return {_data, _label};
	    }
	    
    private:
        std::vector<tensor_blob_like<DType>> _data;
        std::vector<tensor_blob_like<DType>> _label;
	
        std::unordered_map<std::string, std::vector<tensor_blob_like<DType>>> _container_by_label;
	
	    //return: <data,label>
	    std::tuple<std::vector<tensor_blob_like<DType>>, std::vector<tensor_blob_like<DType>>> _get_random_data(int size, const std::vector<tensor_blob_like<DType>>& data_pool, const std::vector<tensor_blob_like<DType>>& label_pool)
	    {
		    const int& total_size = data_pool.size();
		    std::vector<tensor_blob_like<DType>> data,label;
		    data.resize(size);label.resize(size);
		    for (int i = 0; i < size; ++i)
		    {
			    std::random_device dev;
			    std::mt19937 rng(dev());
			    std::uniform_int_distribution<int> distribution(0,total_size-1);
			    int dice = distribution(rng);
			    data[i] = data_pool[dice];
			    label[i] = label_pool[dice];
		    }
		    return {data,label};
	    }
     
	    static uint32_t swap_endian(uint32_t val)
	    {
		    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
		    return (val << 16) | (val >> 16);
	    }
	    
    };
}