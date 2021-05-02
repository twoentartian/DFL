//
// Created by gzr on 01-05-21.
//
#pragma once
#include <sstream>
#include <vector>
#include <memory>
#include <caffe/caffe.hpp>
#include <caffe/solver.hpp>
#include <caffe/net.hpp>
#include <memory>
#include <caffe/layers/memory_data_layer.hpp>
#include "../exception.hpp"
#include <caffe/blob.hpp>
#include "../util.hpp"
#include "./data_converter.hpp"

namespace Ml{
    template <typename DType>
    class data_converter: data_convert<DType>
    {
    public:
        void load_dataset_minist(const std::string &image_filename,const std::string &label_filename){

            std::ifstream data_file(image_filename ,std::ios::in | std::ios::binary);
            std::ifstream label_file(label_filename ,std::ios::in | std::ios::binary);
            CHECK(data_file) << "Unable to open file " << image_filename;
            CHECK(label_file) << "Unable to open file " << label_filename;

            uint32_t magic;
            uint32_t num_items;
            uint32_t num_labels;
            uint32_t rows;
            uint32_t cols;
            data_file.read(reinterpret_cast<char*>(&magic), 4);

            magic = swap_endian(magic);
            CHECK_EQ(magic, 2051) << "Incorrect image file magic.";
            label_file.read(reinterpret_cast<char*>(&magic), 4);
            magic = swap_endian(magic);
            CHECK_EQ(magic, 2049) << "Incorrect label file magic.";
            data_file.read(reinterpret_cast<char*>(&num_items), 4);
            num_items = swap_endian(num_items);
            label_file.read(reinterpret_cast<char*>(&num_labels), 4);
            num_labels = swap_endian(num_labels);
            CHECK_EQ(num_items, num_labels);
            data_file.read(reinterpret_cast<char*>(&rows), 4);
            rows = swap_endian(rows);
            data_file.read(reinterpret_cast<char*>(&cols), 4);
            cols = swap_endian(cols);

            DType labels ;
            DType *pixels = new DType[rows * cols];

            data.resize(num_items);
            label.resize(num_labels);


            for (int i = 0; i < num_items; ++i){
                data_file.read(reinterpret_cast <char*>(pixels), rows*cols);



            }
            for (int i =0;i<num_labels;++i){
                label_file.read(reinterpret_cast <char*>(labels), 1);
            }




            delete[] labels;
            delete[] pixels;

        }
        static uint32_t swap_endian(uint32_t val) {
            val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
            return (val << 16) | (val >> 16);
        }

        const tensor_blob_like<DType>& get_data() const {
            data.getData().resize(10);
            return data;
        }

        const tensor_blob_like<DType>& get_label() const {
            return label;
        }


    private:


    };
}