
namespace Ml {

    template<typename DType>
    class tensor_blob_like {
    public:
        tensor_blob_like() : _empty(true) {};

        void fromBlob(const caffe::Blob <DType> &blob) {
            //shape
            _shape = blob.shape();

            _empty = blob.shape().empty();

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

        bool empty() {
            return _empty;
        }

        GENERATE_GET(_shape, getShape
        );
        GENERATE_GET(_data, getData
        );
    private:
        friend class boost::serialization::access;

        std::vector<int> _shape;
        std::vector <DType> _data;
        bool _empty;
    };

}