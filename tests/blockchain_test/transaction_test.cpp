#include "time_util.hpp"

#include "../../bin/transaction.hpp"

#define BOOST_TEST_MAIN

#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_SUITE (transaction_test)
	
	BOOST_AUTO_TEST_CASE (transaction_md5_test)
	{
		node_info info;
		info.node_address = "name";
		info.node_pubkey = "pubkey";
		transaction_without_hash_sig trans;
		trans.creator = info;
		trans.creation_time = 1000;
		trans.expire_time = 1600;
		trans.ttl = 5;
		byte_buffer buffer;
		trans.to_byte_buffer(buffer);
		crypto::hex_data md5 = crypto::md5::digest_s(buffer.data(), buffer.size());
		std::string md5_str = md5.getTextStr_lowercase();
		BOOST_CHECK(md5_str == "52c403cb855caf0c09567093cbd1bb04");
	}
	
	BOOST_AUTO_TEST_CASE (transaction_sha256_test)
	{
		node_info info;
		info.node_address = "name";
		info.node_pubkey = "pubkey";
		transaction_without_hash_sig trans;
		trans.creator = info;
		trans.creation_time = 1000;
		trans.expire_time = 1600;
		trans.ttl = 5;
		byte_buffer buffer;
		trans.to_byte_buffer(buffer);
		crypto::hex_data sha256 = crypto::sha256::digest_s(buffer.data(), buffer.size());
		std::string sha256_str = sha256.getTextStr_lowercase();
		BOOST_CHECK(sha256_str == "53a54f721925d075e5c5d19f35d065edafb1ba77fd42ea34a3a3d54634101bc7");
	}
	
	BOOST_AUTO_TEST_CASE (transaction_json_print)
	{
		auto [pubKey, priKey] = crypto::ecdsa_openssl::generate_key_pairs();
	
		node_info info;
		info.node_address = "name";
		info.node_pubkey = "pubkey";
		transaction_without_hash_sig trans_;
		trans_.creator = info;
		trans_.creation_time = 1000;
		trans_.expire_time = 1600;
		trans_.ttl = 5;
		trans_.model_data = "model";
		trans_.accuracy = "0.5";
		
		transaction trans;
		trans.content = trans_;
		{
			byte_buffer buffer;
			trans_.to_byte_buffer(buffer);
			auto hash = crypto::sha256::digest_s(buffer.data(), buffer.size());
			trans.hash_sha256 = hash.getTextStr_lowercase();
			auto signature = crypto::ecdsa_openssl::sign(hash, priKey);
			trans.signature = signature.getTextStr_lowercase();
		}
		
		transaction_receipt receipt;
		receipt.content.receive_at_ttl = 5;
		receipt.content.accuracy = "0.5";
		receipt.content.creator = info;
		receipt.content.creation_time = 1010;
		{
			byte_buffer buffer;
			receipt.content.to_byte_buffer(buffer);
			auto hash = crypto::sha256::digest_s(buffer.data(), buffer.size());
			receipt.hash_sha256 = hash.getTextStr_lowercase();
			auto signature = crypto::ecdsa_openssl::sign(hash, priKey);
			receipt.signature = signature.getTextStr_lowercase();
		}
		trans.receipts.push_back(receipt);
		trans.receipts.push_back(receipt);
		
		auto json = trans.to_json();
		std::cout << json.dump(4);
		
	}

BOOST_AUTO_TEST_SUITE_END()