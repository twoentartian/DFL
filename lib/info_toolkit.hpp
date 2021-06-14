
// Created by gzr on 10-06-21.


#pragma once
#include <crypto/ecdsa_openssl.hpp>
#include <crypto/hex_data.hpp>
#include <iostream>
using namespace std;
class info_toolkit{

    public:
        info_toolkit(): pubkey(""), prvkey("")
        {
            auto keypair = crypto::ecdsa_openssl::generate_key_pairs();
            this->pubkey = std::get<0>(keypair);
            this->prvkey = std::get<1>(keypair);
        };

        info_toolkit(const crypto::hex_data pubkey, const crypto::hex_data prikey):pubkey(pubkey), prvkey(prikey){};
        info_toolkit(const std::tuple<crypto::hex_data, crypto::hex_data>keypair): pubkey(std::get<0>(keypair)),prvkey(std::get<1>(keypair)){};
        void print_pubkey()

        {
            if(pubkey.getHexMemory().empty())
            {
             throw std::runtime_error("failed to find available keys");
            }
            else
            {
               cout<<pubkey.getTextStr_lowercase().c_str()<<endl;

            }
        }

    private:
        crypto::hex_data pubkey;
        crypto::hex_data prvkey;

};
