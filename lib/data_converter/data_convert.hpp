//
// Created by gzr on 01-05-21.
//
#pragma once
#include <sstream>
#include <vector>
#include <memory>
#include <fstream>
#include <glog/logging.h>

#include "./tensor_blob_like.hpp"
#include "../exception.hpp"
#include "../util.hpp"

namespace Ml{
    template <typename DType>
    class data_converter
    {
    public:
        void load_dataset_mnist(const std::string &image_filename, const std::string &label_filename) {
            std::ifstream data_file(image_filename, std::ios::in | std::ios::binary);
            std::ifstream label_file(label_filename, std::ios::in | std::ios::binary);
            CHECK(data_file) << "Unable to open file " << image_filename;
            CHECK(label_file) << "Unable to open file " << label_filename;

            uint32_t magic;
            uint32_t num_items;
            uint32_t num_labels;
            uint32_t rows;
            uint32_t cols;
            data_file.read(reinterpret_cast<char *>(&magic), 4);
            magic = swap_endian(magic);
            CHECK_EQ(magic, 2051) << "Incorrect image file magic.";
            label_file.read(reinterpret_cast<char *>(&magic), 4);
            magic = swap_endian(magic);
            CHECK_EQ(magic, 2049) << "Incorrect label file magic.";
            data_file.read(reinterpret_cast<char *>(&num_items), 4);
            num_items = swap_endian(num_items);

            label_file.read(reinterpret_cast<char *>(&num_labels), 4);
            num_labels = swap_endian(num_labels);
            CHECK_EQ(num_items, num_labels);
            data_file.read(reinterpret_cast<char *>(&rows), 4);
            rows = swap_endian(rows);
            data_file.read(reinterpret_cast<char *>(&cols), 4);
            cols = swap_endian(cols);

            std::vector<uint32_t> data_shape{num_items, 1, rows, cols};
            std::vector<uint32_t> label_shape{num_labels, 1, 1, 1};
            data.getShape().insert(data.getShape().end(), data_shape.begin(), data_shape.end());
            label.getShape().insert(label.getShape().end(), data_shape.begin(), data_shape.end());
            DType labels;
            std::vector<DType> pixels;
            std::vector<uint8_t> buffer;
            pixels.resize(rows * cols);
            buffer.resize(rows * cols);
            for (int i = 0; i < num_items; ++i) {
                data_file.read(reinterpret_cast <char *>(buffer.data()), rows * cols);
                for (int data_index = 0; data_index < rows * cols; ++data_index) {
                    pixels[data_index] = static_cast<DType>(buffer[data_index]) * 0.00390625f;
                }
                data.getData().insert(data.getData().end(), pixels.begin(), pixels.end());

                uint8_t label_buffer;
                label_file.read(reinterpret_cast <char *>(&label_buffer), 1);
                label.getData().push_back(label_buffer);
            }


        }


        static uint32_t swap_endian(uint32_t val)
        {
            val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
            return (val << 16) | (val >> 16);
        }

        const tensor_blob_like<DType>& get_data() const
        {
            return data;
        }

        const tensor_blob_like<DType>& get_label() const
        {
            return label;
        }


    private:
        tensor_blob_like<DType> data;
        tensor_blob_like<DType> label;


    };
}