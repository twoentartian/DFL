//
// Created by gzr on 08-07-21.
//

#include "../../bin/data_parsing.hpp"
#include "../../bin/tool/lz4.hpp"
int main()
{

    uint8_t command = '2';
    constexpr int size = 500;
    std::string data = util::get_random_str(size);
    std::stringstream ss;
    ss << command << data;
    std::string data_end = ss.str();
    data_medium process;
    auto type = std::get<0>(process.get_rawdata(data_end));
    auto data_result = std::get<1>(process.get_rawdata(data_end));
    CHECK_EQ(type, command) << "pass faild.";
    CHECK_EQ(data, data_result) << "pass faild.";

    LZ4 compress;
    auto dstsize = compress.Compress_CalculateDstSize(data.length());
    uint8_t command1 = '3';
    uint8_t *buffer = new uint8_t [dstsize];
    auto finished = compress.Compress(data.c_str(), data.size(), reinterpret_cast<char *>(buffer), dstsize);
    std::string s1( (char *) buffer);
    std::stringstream ss1;
    ss1 << command1 << s1;
    auto data2 = ss1.str();
    auto type1 = std::get<0>(process.data_decompressed(data2));
    auto data_result1 = std::get<1>(process.data_decompressed(data2));
    CHECK_EQ(type1, command1) << "pass faild.";
    CHECK_EQ(data, data_result1) << "pass faild.";

    delete[] buffer;

    return 0;

}