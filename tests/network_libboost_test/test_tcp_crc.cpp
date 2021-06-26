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
        auto ip = "127.0.0.1";
        auto port = 88;
        network::socket_header h(ip, port, network::DATATYPE::hex, data);
        network::socket_header t(ip, port, network::DATATYPE::transtion_without_hash, buffer.data(), buffer.size());
        network::socket_header ts(ip, port, network::DATATYPE::transtion_without_hash, trans_binary_str);
        auto h1 = h.get_header();
        auto h2 = t.get_header();
        auto t2 = ts.get_header();


    return 0;
}