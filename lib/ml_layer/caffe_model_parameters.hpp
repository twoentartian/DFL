
#pragma once

#include <memory>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>

#include <caffe/solver.hpp>
#include <caffe/net.hpp>
#include <caffe/layer.hpp>
#include <ml_layer/ml_abs.hpp>

#include <util.hpp>
#include "tensor_blob_like.hpp"

namespace Ml
{
    template <typename DType>
    class caffe_parameter_layer;
    template <typename DType>
    class caffe_parameter_net;
    class caffe_net_structure_layer;
    class caffe_net_structure;

    template <typename DType>
    class caffe_parameter_layer
    {
    public:
        caffe_parameter_layer() = default;

        void fromLayer(const caffe::Layer<DType>& layer)
        {
            _name = layer.layer_param().name();
            _type = layer.layer_param().type();
            auto* layer_without_const = const_cast<caffe::Layer<DType>*>(&layer);
            auto& blobs = layer_without_const->blobs();
            _blob_p.reset(new tensor_blob_like<DType>());
            if (blobs.size() == 0)
            {
                return;
            }
            else
            {
                auto& blob_model = *(blobs[0]);//blobs 0 stores the model data, blobs 1 stores the output blob.
                _blob_p->fromBlob(blob_model);
            }
        }

        void toLayer(caffe::Layer<DType>& layer, bool reshape) const
        {
            if (layer.layer_param().name() != _name)
            {
                LOG(WARNING) << "layer name mismatch: " << layer.layer_param().name() << " != (this)" << _name;
            }
            if (layer.layer_param().type() != _type)
            {
                LOG(WARNING) << "layer type mismatch: " << layer.layer_param().type() << " != (this)" << _type;
            }

            auto& blobs = layer.blobs();
            //empty?
            if (_blob_p->empty())
            {
                blobs.clear();
                return;
            }
            //ensure the target blob is not empty.
            if (blobs.empty())
            {
                LOG(WARNING) << "layer "<< _name <<"'s blob is empty";
            }
            _blob_p->toBlob(*(blobs[0]), reshape);
        }

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & _name;
            ar & _type;
            ar & _blob_p;
        }

        GENERATE_GET(_name, getName);
        GENERATE_GET(_type, getType);
        GENERATE_GET(_blob_p, getBlob_p);
    private:
        friend class boost::serialization::access;

        std::string _name;
        std::string _type;
        boost::shared_ptr<tensor_blob_like<DType>> _blob_p;
    };

    template <typename DType>
    class caffe_parameter_net
    {
    public:
        caffe_parameter_net() = default;

        void fromNet(const caffe::Net<DType>& net)
        {
            _name = net.name();
            _layers.clear();
            _layers.resize(net.layers().size());
            for (int i = 0; i < net.layers().size(); ++i)
            {
                _layers[i].fromLayer(*(net.layers()[i]));
            }
        }

        void toNet(caffe::Net<DType>& net, bool reshape = false) const
        {
            if (net.name() != _name)
            {
                LOG(WARNING) << "net name mismatch: " << net.name() << " != (this)" << _name;
            }
            if (net.layers().size() != _layers.size())
            {
                LOG(WARNING) << "net layer size mismatch: " << net.layers().size() << " != (this)" << _layers.size();
            }
            auto& layers_p = net.layers();

            for (int i = 0; i < _layers.size(); ++i)
            {
                auto& target_layer = *(layers_p[i]);
                _layers[i].toLayer(target_layer, reshape);
            }
        }

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & _name;
            ar & _layers;
        }

        GENERATE_GET(_name, getName);
        GENERATE_GET(_layers, getLayers);
    private:
        friend class boost::serialization::access;

        std::string _name;
        std::vector<caffe_parameter_layer<DType>> _layers;
    };

    class caffe_net_structure_layer
    {
    public:
        caffe_net_structure_layer() = default;

        template<typename DType>
        void fromLayer(const caffe_parameter_layer<DType>& layer)
        {
            _name = layer.getName();
            _type = layer.getType();
            _shape.clear();
            if (layer.getBlob_p())
            {
                _shape = layer.getBlob_p()->getShape();
            }
        }

        template<typename DType>
        void fromLayer(const caffe::Layer<DType>& layer)
        {
            _name = layer.layer_param().name();
            _type = layer.layer_param().type();
            auto* layer_without_const = const_cast<caffe::Layer<DType>*>(&layer);
            auto& blobs = layer_without_const->blobs();
            _shape.clear();
            if (blobs.size() == 0)
            {
                return;
            }
            else
            {
                auto& blob_model = *(blobs[0]);//blobs 0 stores the model data, blobs 1 stores the output blob.
                _shape = blob_model.shape();
            }
        }

        bool operator == (const caffe_net_structure_layer& target) const
        {
            if (target._name != this->_name)
            {
                return false;
            }
            if (target._shape != this->_shape)
            {
                return false;
            }
            return true;
        }

        bool operator != (const caffe_net_structure_layer& target) const
        {
            return !this->operator==(target);
        }

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & _name;
            ar & _type;
            ar & _shape;
        }

        GENERATE_GET(_name, getName);
        GENERATE_GET(_type, getType);
        GENERATE_GET(_shape, getShape);
    private:
        friend class boost::serialization::access;

        std::string _name;
        std::string _type;
        std::vector<int> _shape;
    };

    class caffe_net_structure
    {
    public:
        template<typename DType>
        caffe_net_structure(caffe_parameter_net<DType> net)
        {
            _name = net.getName();
            if constexpr (std::is_same_v<DType, float>)
            {
                _dataType = model_data_type::data_type_float;
            }
            if constexpr (std::is_same_v<DType, double>)
            {
                _dataType = model_data_type::data_type_double;
            }
            auto& net_layers = net.getLayers();
            _layers.resize(net_layers.size());
            for (int i = 0; i < _layers.size(); ++i)
            {
                _layers[i].fromLayer(net_layers[i]);
            }
        }

        bool operator == (const caffe_net_structure& target) const
        {
            if (_layers.size() != target._layers.size())
            {
                return false;
            }
            for (int i = 0; i < _layers.size(); ++i)
            {
                if (_layers[i] != target._layers[i])
                {
                    return false;
                }
            }
            return true;
        }

        bool operator != (const caffe_net_structure& target) const
        {
            return !this->operator==(target);
        }

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & _name;
            ar & _layers;
        }

        GENERATE_GET(_name, getName);
        GENERATE_GET(_layers, getLayers);
    private:
        friend class boost::serialization::access;

        std::string _name;
        model_data_type _dataType;
        std::vector<caffe_net_structure_layer> _layers;
    };
}