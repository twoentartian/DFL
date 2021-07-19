#pragma once

#include <tuple>
#include <string>

#include <boost/format.hpp>

#include <crypto.hpp>

#include "transaction.hpp"

std::tuple<bool, std::string> verify_generator(const node_info& node, const crypto::hex_data& signature = crypto::hex_data(), const crypto::hex_data& sha256 = crypto::hex_data())
{
	bool pass = true;
	
	// verify public key & address
	crypto::hex_data public_key(node.node_pubkey);
	crypto::hex_data node_address(node.node_address);
	auto public_key_hex_memory = public_key.getHexMemory();
	auto calculated_node_address = crypto::sha256::digest_s(public_key_hex_memory.data(), public_key_hex_memory.size());
	pass = calculated_node_address == node_address;
	if (!pass)
		return {pass, (boost::format("node address(%1%) != calculated(%2%)") % node_address.getTextStr_lowercase() % calculated_node_address.getTextStr_lowercase()).str()};
	
	// verify signature
	if ((!signature.getHexMemory().empty()) && (!sha256.getHexMemory().empty()))
	{
		pass = crypto::ecdsa_openssl::verify(signature, sha256, public_key);
		if (!pass)
			return {pass, (boost::format("signature:(%1%) verify failed") % signature.getTextStr_lowercase()).str()};
	}
	
	return {pass, ""};
}

//TODO: verify more information in the transaction
std::tuple<bool, std::string> verify_transaction(const transaction& trans)
{
	bool pass = true;
	
	//verify time
	auto time_now = time_util::get_current_utc_time();
	if (time_now < trans.content.creation_time - 10)
	{
		pass = false;
		return {pass, (boost::format("transaction(%3%), creation time(%1%) - 10 > now(%2%)") % trans.content.creation_time % time_now % trans.hash_sha256).str() };
	}
	if (time_now > trans.content.expire_time)
	{
		pass = false;
		return {pass, (boost::format("transaction(%3%), expired time(%1%) < now(%2%)") % trans.content.expire_time % time_now % trans.hash_sha256).str() };
	}
	
	//verify transaction hash
	crypto::hex_data sha256_hash(trans.hash_sha256);
	byte_buffer buffer;
	trans.content.to_byte_buffer(buffer);
	crypto::hex_data calculated_hash = crypto::sha256::digest_s(buffer.data(), buffer.size());
	pass = calculated_hash == sha256_hash;
	if (!pass)
		return {pass, "transaction hash mismatch, raw(" + sha256_hash.getTextStr_lowercase() + ") != calculated(" + calculated_hash.getTextStr_lowercase() + ")"};
	
	//verify transaction generator
	crypto::hex_data signature(trans.signature);
	auto [status, message] = verify_generator(trans.content.creator, signature, sha256_hash);
	if (!status)
		return {status, (boost::format("transaction(%2%), %1%") % message % trans.hash_sha256).str()};
	
	//verify receipt
	for (auto& [receipt_hash, receipt]: trans.receipts)
	{
		//verify time
		if (time_now < receipt.content.creation_time - 10)
		{
			pass = false;
			return {pass, (boost::format("transaction(%3%), receipt (%4%), creation time(%1%) - 10 > now(%2%)") % trans.content.creation_time % time_now % trans.hash_sha256 % receipt.hash_sha256).str() };
		}
		
		//verify transaction generator
		auto [status, message] = verify_generator(trans.content.creator, signature, sha256_hash);
		if (!status)
			return {status, (boost::format("transaction(%2%), receipt (%3%), %1%") % message % trans.hash_sha256 % receipt.hash_sha256).str()};
	}
	
	return {pass, ""};
}