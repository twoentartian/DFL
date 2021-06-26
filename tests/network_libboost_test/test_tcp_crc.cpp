//
// Created by gzr on 24-06-21.

#include<network-common/crc.hpp>
#include "../../bin/transaction.hpp"
#include <crypto.hpp>
#include <hash_interface.hpp>
#include <boost_serialization_wrapper.hpp>
#include<glog/logging.h>

int main()
{

        node_info info;
        info.node_address = "name";
        info.node_pubkey = "pubkey";
        transaction_without_hash_sig trans;
        trans.creator = info;
        trans.creation_time = 1000;
        trans.expire_time = 1600;
        trans.TTL = 5;
        byte_buffer buffer;
        trans.to_byte_buffer(buffer);
        crypto::hex_data bu = crypto::hex_data(buffer.data(), buffer.size());
        auto bus = bu.getTextStr_lowercase();
        std::string trans_binary_str = serialize_wrap<boost::archive::binary_oarchive>(trans).str();
        auto data = "this is a testing";

        network::socket_header h(network::DATATYPE::hex, data);
        network::socket_header t(network::DATATYPE::transtion_without_hash, buffer.size());
        network::socket_header ts(network::DATATYPE::transtion_without_hash, trans_binary_str);
        auto h1 = h.get_header();
        auto h2 = t.get_header();
        auto t2 = ts.get_header();
        byte_buffer bufferh1;
        h.get_header_buffer(h1,bufferh1);
        auto h1_result = network::decode_header(bufferh1.data(),bufferh1.size());

        byte_buffer bufferh2;
        h.get_header_buffer(h2,bufferh2);
        auto h2_result = network::decode_header(bufferh2.data(),bufferh2.size());
        CHECK_EQ(1,h1_result);
        CHECK_EQ(1,h2_result);

        byte_buffer bufferh3;
        uint8_t n = 'c';
        uint8_t m = 'm';
        bufferh3.add(n);
        bufferh3.add(m);
        h.get_header_buffer(h1,bufferh3);
        auto h3_result = network::decode_header(bufferh3.data(),bufferh3.size());
        CHECK_EQ(3,h3_result);
    return 0;
}