//
// Created by gzr on 06-07-21.
//
#pragma once
#include <ml_layer.hpp>
#include <lz4.hpp>
#include "boost/variant.hpp"

class data_medium
{
    public:
        using data_command = uint8_t;
        //0:parameter_raw,1:parameter_compressed,2:difference_raw,3:difference_compressed
        std::tuple<data_command, std::string> data_decompressed(const std::string & data)
        {
            // return the command and raw data
            data_command parameter_command = data[0];
            std::string compressed_data = data.substr(1);
            LZ4 decompressor;
            auto dstsize = decompressor.Decompress_CalculateDstSize(compressed_data.data());
            char *buffer = new char [dstsize];
            auto finish = decompressor.Decompress(compressed_data.data(), compressed_data.size(), buffer);
            if(finish)
            {
                return {parameter_command,buffer};
            }
            else
            {
                LOG(ERROR) << "failed to decompressed";
            }
            delete[] buffer;
        }

        std::tuple<data_command, std::string> data_decompressed(const uint8_t *data)
        {
            std::string s( (char *) data);
            return data_decompressed(s);
        }

        std::tuple<data_command,std::string> get_rawdata(const std::string & data)
        {
            return{data[0],data.substr(1)};
        }


        std::tuple<data_command,std::string> get_rawdata(const uint8_t *data)
        {
            std::string s( (char *) data);
            return get_rawdata(s);
        }

};

