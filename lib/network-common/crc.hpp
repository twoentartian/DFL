//
// Created by gzr on 24-06-21.
//
#pragma once
#include <boost/crc.hpp>
#include <memory>

namespace network
{

    enum DATATYPE
    {
        transtion,
        transtion_without_hash,
        network_structure,
        network_structure_layer,
        network_net_parameter,
        network_net_layer_parameter,
        hex
    };
    struct header
    {
        std::string ip;
        uint16_t port;
        size_t length;
        uint16_t CRC;
        DATATYPE inst;
    };

    class socket_header{
        public:
            header H;
            socket_header(std::string ip,uint16_t port,DATATYPE type,const std::string data)
            {H.ip = ip;H.port=port;H.inst=type;H.CRC = GetCrc16(reinterpret_cast<const uint8_t*>(data.data()), data.size());H.length=sizeof(data);};

            socket_header(std::string ip,uint16_t port,DATATYPE type,const uint8_t *data,size_t data_size)
            {H.ip = ip;H.port=port;H.inst=type;H.CRC = GetCrc16(data,data_size);H.length=data_size;};

            header *get_header()
            {
                return &H;
            }
            void reset_all(std::string ip,uint16_t port,DATATYPE type,const std::string data)
            {
                H.ip = ip;H.port=port;H.inst=type;H.CRC = GetCrc16(reinterpret_cast<const uint8_t*>(data.data()), data.size());H.length=sizeof(data);
            }
            void reset_port(uint16_t port)
            {
                H.port = port;
            }
            void reset_address(std::string ip)
            {
                H.ip = ip;
            }

            void reset_data(const std::string data,DATATYPE newtype)
            {
                H.CRC = GetCrc16(reinterpret_cast<const uint8_t*>(data.data()), data.size());
                H.length=sizeof(data);
                H.inst = newtype;
            }
            void reset_data(const uint8_t *data,size_t data_size,DATATYPE newtype)
            {
                H.CRC = GetCrc16(data,data_size);
                H.length=data_size;
                H.inst = newtype;
            }
        private:
            uint16_t GetCrc16(const uint8_t *data,size_t size)
            {
                boost::crc_16_type result;
                result.process_bytes(data, size);
                return result.checksum();
            }
    };
}
