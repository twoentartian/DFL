#pragma once
#ifndef C_TENSOR_BLOB_LIKE_H
#define C_TENSOR_BLOB_LIKE_H

#include <sstream>
#include <random>
#include <boost/serialization/access.hpp>
#include <caffe/data_transformer.hpp>
#include <boost/container_hash/hash.hpp>
#include <boost/functional/hash.hpp>
#include "./util.hpp"

namespace Ml {
	class tensor_blob_like_abs
	{
	public:
		tensor_blob_like_abs() = default;
	};
	
    template<typename DType>
    class tensor_blob_like : public tensor_blob_like_abs {
    public:
        tensor_blob_like() = default;
	
	    using DataType = DType;

        void fromBlob(const caffe::Blob <DType> &blob) {
            //shape
            _shape = blob.shape();

            //data
            _data.resize(blob.count());
            const DType *data_vec = blob.cpu_data();
            std::memcpy(_data.data(), data_vec, _data.size() * sizeof(DType));
        }

        //exception: std::runtime_error("blob shape does not match")
        void toBlob(caffe::Blob <DType> &blob, bool reshape) {
            //shape
            if (reshape) {
                blob.Reshape(_shape);
            } else {
                if (blob.shape() != _shape) {
                    throw std::runtime_error("blob shape does not match");
                }
            }

            //data
            DType *data_vec = blob.mutable_cpu_data();
            std::memcpy(data_vec, _data.data(), _data.size() * sizeof(DType));
        }
        
        //warning: vector comparison is only available in C++ 20
        bool operator == (const tensor_blob_like& target) const
        {
	        return (target._shape == this->_shape) && (target._data == this->_data);
        }
		
	    //warning: vector comparison is only available in C++ 20
	    bool operator != (const tensor_blob_like& target) const
	    {
		    return !((target._shape == this->_shape) && (target._data == this->_data));
	    }
	
	    tensor_blob_like<DType> operator+(const tensor_blob_like<DType>& target) const
	    {
		    tensor_blob_like<DType> output = *this;
		    if(target._shape != this->_shape)
		    {
			    throw std::invalid_argument("tensor/blob shape mismatch");
		    }
		    for(int i = 0; i < output._data.size(); i++)
		    {
			    output._data[i] += target._data[i];
		    }
		    return output;
	    }
	
	    tensor_blob_like<DType> operator-(const tensor_blob_like<DType>& target) const
	    {
		    tensor_blob_like<DType> output = *this;
		    if(target._shape != this->_shape)
		    {
			    throw std::invalid_argument("tensor/blob shape mismatch");
		    }
		    for(int i = 0; i < output._data.size(); i++)
		    {
			    output._data[i] -= target._data[i];
		    }
		    return output;
	    }
	
	    template<typename D>
	    tensor_blob_like<DType> operator/(const D& target) const
	    {
		    tensor_blob_like<DType> output = *this;
		    for(int i = 0; i < output._data.size(); i++)
		    {
			    output._data[i] /= target;
		    }
		    return output;
	    }
	
	    template<typename D>
	    tensor_blob_like<DType> operator*(const D& target) const
	    {
		    tensor_blob_like<DType> output = *this;
		    for(int i = 0; i < output._data.size(); i++)
		    {
			    output._data[i] *= target;
		    }
		    return output;
	    }
	
	    [[nodiscard]] tensor_blob_like<DType> dot_divide(const tensor_blob_like<DType>& target) const
	    {
		    tensor_blob_like<DType> output = *this;
		    if(target._shape != this->_shape)
		    {
			    throw std::invalid_argument("tensor/blob shape mismatch");
		    }
		    for(int i = 0; i < output._data.size(); i++)
		    {
			    output._data[i] /= target._data[i];
		    }
		    return output;
	    }
	
	    [[nodiscard]] tensor_blob_like<DType> dot_product(const tensor_blob_like<DType>& target) const
	    {
		    tensor_blob_like<DType> output = *this;
		    if(target._shape != this->_shape)
		    {
			    throw std::invalid_argument("tensor/blob shape mismatch");
		    }
		    for(int i = 0; i < output._data.size(); i++)
		    {
			    output._data[i] *= target._data[i];
		    }
		    return output;
	    }

        bool empty() {
            return _data.empty();
        }

        GENERATE_GET(_shape, getShape);
        GENERATE_GET(_data, getData);
	    
        void swap(tensor_blob_like& target)
        {
	        this->_data.swap(target._data);
	        this->_shape.swap(target._shape);
        }
        
        void set_all(DType value)
        {
	        for (auto& single_value: _data)
	        {
				single_value = value;
	        }
        }
        
        void random(DType min, DType max)
        {
        	std::random_device rd;
        	std::uniform_real_distribution distribution(min, max);
	        for (auto& single_value: _data)
	        {
		        single_value = distribution(rd);
	        }
        }
        
        bool roughly_equal(const tensor_blob_like& target, DType diff_threshold) const
        {
	        if (target._shape != this->_shape)
	        {
		        return false;
	        }
	        for (int i = 0; i < this->_data.size(); ++i)
	        {
		        if (diff_threshold < std::abs(this->_data[i] - target._data[i])) return false;
	        }
	        return true;
        }
	
	    void patch_weight(const tensor_blob_like<DType>& patch, DType ignore = NAN)
	    {
		    if(patch._shape != this->_shape)
		    {
			    throw std::invalid_argument("tensor/blob shape mismatch");
		    }
		    if (isnan(ignore))
		    {
			    for(int i = 0; i < _data.size(); i++)
			    {
				    if (!isnan(patch._data[i]))
				    {
					    _data[i] = patch._data[i];
				    }
			    }
		    }
		    else
		    {
			    for(int i = 0; i < _data.size(); i++)
			    {
				    if (patch._data[i] != ignore)
				    {
					    _data[i] = patch._data[i];
				    }
			    }
		    }
	    }
	
	    [[nodiscard]] std::string get_str() const
	    {
			std::stringstream output;
		    for (auto&& shape: _shape)
		    {
			    output << std::to_string(shape);
		    }
		    for (auto&& data: _data)
		    {
			    output << std::to_string(data);
		    }
		    return output.str();
	    }
	    
	    DType sum()
	    {
            DType output = 0;
		    for (const auto& value: _data)
		    {
				output += value;
		    }
		    return output;
	    }
	    
	    void abs()
	    {
		    for (auto& value: _data)
		    {
			    if (value < 0 ) value = -value;
		    }
	    }
	    
	    size_t size() const
	    {
		    return _data.size();
	    }
	
	    void regulate_weights(DType min, DType max)
	    {
		    for (auto& value: _data)
		    {
			    if (value < min) value = min;
			    if (value > max) value = max;
		    }
		}
		
		void fix_nan()
		{
			for (auto& value: _data)
			{
				if (isnan(value)) value = 0;
			}
		}
     
    private:
        friend class boost::serialization::access;
	    template<class Archive>
	    void serialize(Archive &ar, const unsigned int version) {
		    ar & _shape;
		    ar & _data;
	    }
        
        std::vector<int> _shape;
        std::vector<DType> _data;
    };
	
	struct tensor_blob_like_hasher
	{
		std::size_t operator()(const Ml::tensor_blob_like<float>& k) const
		{
			using std::string;
			std::string str = k.get_str();
			return std::hash<string>()(str);
		}
		
		std::size_t operator()(const Ml::tensor_blob_like<double>& k) const
		{
			using std::string;
			std::string str = k.get_str();
			return std::hash<string>()(str);
		}
	};
 
}

#endif