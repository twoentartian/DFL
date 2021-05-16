
#ifndef C_TENSOR_BLOB_LIKE_H
#define C_TENSOR_BLOB_LIKE_H

#include <boost/serialization/access.hpp>
#include <caffe/data_transformer.hpp>
#include "./util.hpp"

namespace Ml {

    template<typename DType>
    class tensor_blob_like {
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


        void fromDatum(const caffe::Datum &datum){
            //_empty = datum.has_data();
            caffe::Blob<DType>* transformed_blob;
            caffe::DataTransformer<DType> transformer;
            transformer.Transform(datum,transformed_blob);
            fromBlob(transformed_blob);

        }
        void toDatum(caffe::Datum &datum,bool reshape,bool label){
            std::vector<int> datum_shape;
            datum_shape[0] = (datum.has_data())?datum.data().size():0;
            datum_shape[1] = (datum.has_channels()) ? datum.channels() : 0;
            datum_shape[2] = (datum.has_height()) ? datum.height() : 0;
            datum_shape[3] = (datum.has_width()) ? datum.width() : 0;

            if (reshape){
                datum.set_channels(_shape[1]);
                datum.set_height(_shape[2]);
                datum.set_width(_shape[3]);

            }else{
                if(datum_shape!=_shape){
                    throw std::runtime_error("datum shape does not match");
                }

            }

            if (label){
                datum.set_label(_data.data(),_data.size());
            }
            else{
                datum.set_data(_data.data(),_data.size());
            }

        }


        template<class Archive>
        void serialize(Archive &ar, const unsigned int version) {
            ar & _shape;
            ar & _data;
        }

        bool empty() {
            return _data.empty();
        }

        GENERATE_GET(_shape, getShape
        );
        GENERATE_GET(_data, getData
        );
    private:
        friend class boost::serialization::access;

        std::vector<int> _shape;
        std::vector <DType> _data;
    };

}

#endif