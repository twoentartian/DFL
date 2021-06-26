//
// Created by gzr on 24-06-21.
//
#pragma once
#include <boost/crc.hpp>
#include <memory>
#include <cstring>
#include <byte_buffer.hpp>
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
        uint32_t length;
        //uint16_t data_CRC;
        uint16_t inst;
        uint16_t header_CRC;
    };




    class socket_header{
        public:
            header H;
            socket_header(uint16_t type,const std::string data)
            {H.inst=type;//H.data_CRC = GetCrc16_data(reinterpret_cast<const uint8_t*>(data.data()), data.size());
            H.length=sizeof(data);
            H.header_CRC= GetCrc16_header(H);
            }

            socket_header(uint16_t type,size_t data_size)
            {H.inst=type;//H.data_CRC = GetCrc16_data(data,data_size);
            H.length=data_size;H.header_CRC= GetCrc16_header(H);};

            header& get_header()
            {
                return H;
            }
            void get_header_buffer(const header HEADER,byte_buffer& buffer)
            {
                buffer.add(HEADER.length);
                buffer.add(HEADER.inst);
                buffer.add(HEADER.header_CRC);

            }

            void reset_all(uint16_t type,const std::string data)
            {
               H.inst=type;//H.data_CRC = GetCrc16_data(reinterpret_cast<const uint8_t*>(data.data()), data.size());
               H.length=sizeof(data);
            }


            void reset_data(const std::string data,uint16_t newtype)
            {
                //H.data_CRC = GetCrc16_data(reinterpret_cast<const uint8_t*>(data.data()), data.size());
                H.length=sizeof(data);
                H.inst = newtype;
            }
            void reset_data(const uint8_t *data,size_t data_size,uint16_t newtype)
            {
                //H.data_CRC = GetCrc16_data(data,data_size);
                H.length=data_size;
                H.inst = newtype;
            }
        private:
            uint16_t GetCrc16_data(const uint8_t *data,size_t size)
            {
                boost::crc_16_type result;
                result.process_bytes(data, size);
                return result.checksum();
            }
            uint16_t GetCrc16_header(const header HEADER)
            {
            boost::crc_16_type result;
            byte_buffer buffer;
            buffer.add(HEADER.length);
            buffer.add(HEADER.inst);
            result.process_bytes(buffer.data(),buffer.size());
            return result.checksum();
            }


    };



    uint32_t decode_header(const uint8_t *packets, size_t size)
    {

        if(size<8)
        {
            std::cout<<"uncompleted header"<<std::endl;
            return 0;
        }
        else
        {
            for(uint32_t i = 0;i<size-7;i++)
            {
                uint32_t len;
                std::memcpy(&len, &packets[i], sizeof(uint32_t));
                uint16_t inst;
                std::memcpy(&inst, &packets[i+4], sizeof(uint16_t));
                uint16_t crc;
                std::memcpy(&crc, &packets[i+6], sizeof(uint16_t));
                socket_header h(inst, len);
                if (crc == h.H.header_CRC)
                {
                    std::cout << "THIS IS A HEADER" << std::endl;
                    std::cout << "position of HEADER is"<<i+1<< std::endl;
                    return i+1;
                }
            }

        }

    }

}

