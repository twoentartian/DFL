#pragma once

#include <glog/logging.h>

#include <lz4.hpp>
#include <boost_serialization_wrapper.hpp>
#include "tensor_blob_like.hpp"
#include "caffe_model_parameters.hpp"

namespace Ml
{
	class model_compress
	{
	public:
		template<typename DType>
		static Ml::caffe_parameter_net<DType> compress_by_diff_get_model(const Ml::caffe_parameter_net<DType>& net_before, const Ml::caffe_parameter_net<DType>& net_after, float filter_limit, size_t* total_weight_count = nullptr, size_t* dropped_weight_count = nullptr)
		{
			Ml::caffe_parameter_net<DType> net_output;
			auto net_diff = net_after - net_before;
			
			if (total_weight_count != nullptr) *total_weight_count = 0;
			if (dropped_weight_count != nullptr) *dropped_weight_count = 0;
			
			//drop models
			auto& layers = net_diff.getLayers();
			for (int i = 0; i < layers.size(); ++i)
			{
				auto& blob = layers[i].getBlob_p();
				auto& blob_data = blob->getData();
				if (blob_data.empty())
				{
					continue;
				}
				auto [max, min] = find_max_min(blob_data.data(), blob_data.size());
				auto range = max - min;
				auto lower_bound = min + range * filter_limit * 0.5;
				auto higher_bound = max - range * filter_limit * 0.5;
				
				int drop_count = 0;
				for (auto&& data: blob_data)
				{
					if(data < higher_bound && data > lower_bound)
					{
						//drop
						drop_count++;
						data = NAN;
					}
				}
				
				if (total_weight_count != nullptr) *total_weight_count += blob_data.size();
				if (dropped_weight_count != nullptr) *dropped_weight_count += drop_count;
			}
			net_output = net_diff.dot_divide(net_diff).dot_product(net_after);
			
			return net_output;
		}
		
		template<typename DType>
		static std::string compress_by_diff_lz_compress(const Ml::caffe_parameter_net<DType>& diff_model)
		{
			auto data_str = serialize_wrap<boost::archive::binary_oarchive>(diff_model).str();
			int max_compressed_size = LZ4::Compress_CalculateDstSize(data_str.size());
			char* compressed_output = new char[max_compressed_size];
			int real_compressed_size = LZ4::Compress(data_str.data(), data_str.size(), compressed_output, max_compressed_size);
			std::string output(compressed_output, real_compressed_size);
			delete[] compressed_output;
			
			return output;
		}
		
		template<typename DType>
		static std::string compress_by_diff(const Ml::caffe_parameter_net<DType>& net_before, const Ml::caffe_parameter_net<DType>& net_after, float filter_limit, size_t* total_weight_count = nullptr, size_t* dropped_weight_count = nullptr)
		{
			auto diff_model = compress_by_diff_get_model(net_before, net_after, filter_limit, total_weight_count, dropped_weight_count);
			auto output = compress_by_diff_lz_compress(diff_model);
			return output;
		}
		
		template<typename DType>
		static Ml::caffe_parameter_net<DType> decompress_by_diff(const std::string& data)
		{
			int decompress_size = LZ4::Decompress_CalculateDstSize(data.data());
			char* decompress_content = new char[decompress_size];
			int real_decompressed_size = LZ4::Decompress(data.data(), data.size(), decompress_content);
			LOG_IF(WARNING, real_decompressed_size!=decompress_size) << "[model decompress] real_decompressed_size not as expected";
			std::stringstream ss;
			ss.write(decompress_content, decompress_size);
			auto output = deserialize_wrap<boost::archive::binary_iarchive, Ml::caffe_parameter_net<DType>>(ss);
			delete[] decompress_content;
			
			return output;
		}
	
	private:
		//return <max, min>
		template<typename T>
		static std::tuple<T, T> find_max_min(T* data, size_t size)
		{
			T max, min;
			max=*data;min=*data;
			for (int i = 0; i < size; ++i)
			{
				if (data[i] > max) max = data[i];
				if (data[i] < min) min = data[i];
			}
			return {max,min};
		}
		
	};
	
	enum model_compress_type
	{
		unknown = 0,
		normal,
		compressed_by_diff
		
	};
	
	template<typename DType>
	class model_interpreter
	{
	public:
		constexpr static int identifier_size = 4;
		
		static std::string generate_model_stream_by_compress_diff(const Ml::caffe_parameter_net<DType>& net_before, const Ml::caffe_parameter_net<DType>& net_after, float filter_limit, size_t* total_weight_count = nullptr, size_t* dropped_weight_count = nullptr)
		{
			std::string output = model_compress::compress_by_diff(net_before, net_after, filter_limit, total_weight_count, dropped_weight_count);
			append_identifier(output, compressed_by_diff);
			return output;
		}
		
		static std::string generate_model_stream_normal(const Ml::caffe_parameter_net<DType>& net)
		{
			auto output = serialize_wrap<boost::archive::binary_oarchive>(net).str();
			append_identifier(output, normal);
			return output;
		}
		
		static std::tuple<Ml::caffe_parameter_net<DType>,model_compress_type> parse_model_stream(const std::string& data)
		{
			assert(data.size() > identifier_size);
			model_compress_type type = unknown;
			Ml::caffe_parameter_net<DType> output_model;
			parse_identifier(data, type);
			std::stringstream ss;
			ss.write(data.data(), data.size() - identifier_size);
			if (type == normal)
			{
				output_model = deserialize_wrap<boost::archive::binary_iarchive, Ml::caffe_parameter_net<DType>>(ss);
			}
			else if(type == compressed_by_diff)
			{
				output_model = model_compress::decompress_by_diff<DType>(ss.str());
			}
			else
			{
				LOG(WARNING) << "unknown model stream type";
			}
			
			return {output_model, type};
		}
	
	private:
		static void append_identifier(std::string& data, model_compress_type type)
		{
			char identifier[identifier_size];
			std::memset(identifier, 0, identifier_size);
			identifier[0] = type;
			data.reserve(data.size() + identifier_size);
			for (char i : identifier)
			{
				data.push_back(i);
			}
		}
		
		static void parse_identifier(const std::string& data, model_compress_type& type)
		{
			char identifier[identifier_size];
			std::memcpy(identifier, &data[data.size()-4], identifier_size);
			type = model_compress_type(identifier[0]);
		}
		
	};
	
}