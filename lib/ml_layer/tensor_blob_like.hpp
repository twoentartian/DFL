
#ifndef C_TENSOR_BLOB_LIKE_H
#define C_TENSOR_BLOB_LIKE_H

#include <sstream>
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

        template<class Archive>
        void serialize(Archive &ar, const unsigned int version) {
            ar & _shape;
            ar & _data;
        }
        
        bool operator == (const tensor_blob_like& target) const
        {
	        return (target._shape == this->_shape) && (target._data == this->_data);
        }
	
	    bool operator != (const tensor_blob_like& target) const
	    {
		    return !(*this == target);
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

        bool empty() {
            return _data.empty();
        }

        GENERATE_GET(_shape, getShape
        );
        GENERATE_GET(_data, getData
        );
	    
        void swap(tensor_blob_like& target)
        {
	        this->_data.swap(target._data);
	        this->_shape.swap(target._shape);
        }
        
        std::string get_hash_str() const
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
     
    private:
        friend class boost::serialization::access;

        std::vector<int> _shape;
        std::vector<DType> _data;
    };

}

#endif