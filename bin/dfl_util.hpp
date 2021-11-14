#pragma once

#include <sstream>
#include <crypto.hpp>
#include "std_output.hpp"

namespace dfl_util
{
	bool verify_private_public_key(const crypto::hex_data &private_key, const crypto::hex_data &public_key)
	{
		//check private key and public key
		std::string data = util::get_random_str();
		auto hash = crypto::md5::digest_s(data);
		auto signature = crypto::ecdsa_openssl::sign(hash, private_key);
		bool pass = crypto::ecdsa_openssl::verify(signature, hash, public_key);
		return pass;
	}
	
	bool verify_private_public_key(const std::string &private_key, const std::string &public_key)
	{
		crypto::hex_data private_key_hex(private_key);
		crypto::hex_data public_key_hex(public_key);
		return verify_private_public_key(private_key_hex, public_key_hex);
	}
	
	bool verify_address_public_key(const crypto::hex_data &address, const crypto::hex_data &public_key)
	{
		auto pubkey_hex = public_key.getHexMemory();
		auto sha256 = crypto::sha256::digest_s(pubkey_hex.data(), pubkey_hex.size());
		return sha256 == address;
	}
	
	bool verify_address_public_key(const std::string &address, const std::string &public_key)
	{
		crypto::hex_data address_hex(address);
		crypto::hex_data public_key_hex(public_key);
		return verify_address_public_key(address_hex, public_key_hex);
	}
	
	bool verify_signature(const crypto::hex_data &public_key, const crypto::hex_data &signature, const crypto::hex_data &hash)
	{
		return crypto::ecdsa_openssl::verify(signature, hash, public_key);
	}
	
	bool verify_signature(const std::string &public_key, const std::string &signature, const std::string &hash)
	{
		crypto::hex_data public_key_hex(public_key);
		crypto::hex_data signature_hex(signature);
		crypto::hex_data hash_hex(hash);
		return verify_signature(public_key_hex, signature_hex, hash_hex);
	}
	
	template <typename T>
	bool verify_hash(const T &target, const crypto::hex_data &hash)
	{
		byte_buffer buffer;
		target.to_byte_buffer(buffer);
		crypto::hex_data hash_calculated = crypto::sha256::digest_s(buffer.data(), buffer.size());
		return hash == hash_calculated;
	}
	
	template <typename T>
	bool verify_hash(const T &target, const std::string &hash)
	{
		crypto::hex_data hash_hex(hash);
		return verify_hash(target, hash_hex);
	}
	
	void print_warning_to_log_stdcout(std::stringstream &ss)
	{
		auto ss_str = ss.str();
		LOG(WARNING) << ss_str;
		std_cout::println(ss_str);
	}
	
	void print_info_to_log_stdcout(std::stringstream &ss)
	{
		auto ss_str = ss.str();
		LOG(INFO) << ss_str;
		std_cout::println(ss_str);
	}
}