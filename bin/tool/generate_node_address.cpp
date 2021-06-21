// Created by gzr on 10-06-21.

#include <crypto/ecdsa_openssl.hpp>
#include <crypto/hex_data.hpp>
#include <iostream>

int main()
{
	auto [pubKey, priKey] = crypto::ecdsa_openssl::generate_key_pairs();
	auto pubKey_hex = pubKey.getHexMemory();
	auto address = crypto::sha256::digest_s(pubKey_hex.data(), pubKey_hex.size());
	
	std::cout << "public key: " << pubKey.getTextStr_lowercase() << std::endl;
	std::cout << "private key: " << priKey.getTextStr_lowercase() << std::endl;
	std::cout << "address: " << address.getTextStr_lowercase() << std::endl;
	
	return 0;
}
