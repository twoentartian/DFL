#pragma once

#include <iostream>
#include <chrono>
#include <string>
#include <unordered_map>

#include <util.hpp>
#include <crypto.hpp>
#include <hash_interface.hpp>
#include <time_util.hpp>
#include <json_serialization.hpp>
#include <boost_serialization_wrapper.hpp>
#include <base64.hpp>

#include "ml_layer.hpp"
#include "node_info.hpp"
#include "dfl_util.hpp"

using TIME_STAMP_TYPE = uint64_t;
class transaction_without_hash_sig : i_hashable, i_json_serialization
{
public:
	TIME_STAMP_TYPE creation_time;
	TIME_STAMP_TYPE expire_time;
	node_info creator;
	int ttl;
	
	std::string model_data;
	std::string accuracy;
	
	void to_byte_buffer(byte_buffer& target) const
	{
		target.add(creation_time);
		target.add(expire_time);
		creator.to_byte_buffer(target);
		target.add(model_data);
		target.add(accuracy);
		target.add(ttl);
	}
	
	i_json_serialization::json to_json() const
	{
		i_json_serialization::json output;
		output["creation_time"] = creation_time;
		output["expire_time"] = expire_time;
		output["creator"] = creator.to_json();
		output["ttl"] = ttl;
		output["model_data"] = Base64::Encode(model_data);
		output["accuracy"] = accuracy;
		
		return output;
	}
	
	void from_json(const i_json_serialization::json& json_target)
	{
		creation_time = json_target["creation_time"];
		expire_time = json_target["expire_time"];
		creator.from_json(json_target["creator"]);
		ttl = json_target["ttl"];
		Base64::Decode(json_target["model_data"], model_data);
		accuracy = json_target["accuracy"];
	}

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & creation_time;
		ar & expire_time;
		ar & creator;
		ar & model_data;
		ar & accuracy;
		ar & ttl;
	}
};

class transaction_receipt_without_hash_sig : i_hashable, i_json_serialization
{
public:
	TIME_STAMP_TYPE creation_time;
	node_info creator;
	std::string accuracy;
	std::string transaction_hash;
	int receive_at_ttl;
	
	void to_byte_buffer(byte_buffer& target) const
	{
		target.add(creation_time);
		creator.to_byte_buffer(target);
		target.add(accuracy);
		target.add(receive_at_ttl);
		target.add(transaction_hash);
	}
	
	i_json_serialization::json to_json() const
	{
		i_json_serialization::json output;
		output["creation_time"] = creation_time;
		output["creator"] = creator.to_json();
		output["accuracy"] = accuracy;
		output["receive_at_ttl"] = receive_at_ttl;
		output["transaction_hash"] = transaction_hash;
		
		return output;
	}
	
	void from_json(const i_json_serialization::json& json_target)
	{
		creation_time = json_target["creation_time"];
		creator.from_json(json_target["creator"]);
		accuracy = json_target["accuracy"];
		receive_at_ttl = json_target["receive_at_ttl"];
		transaction_hash = json_target["transaction_hash"];
	}
	
	bool operator==(const transaction_receipt_without_hash_sig& target) const
	{
		return (creation_time == target.creation_time) &&
				(creator == target.creator) &&
				(accuracy == target.accuracy) &&
				(receive_at_ttl == target.receive_at_ttl) &&
				(transaction_hash == target.transaction_hash);
	}
	
	bool operator!=(const transaction_receipt_without_hash_sig& target) const
	{
		return !operator==(target);
	}
	
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & creation_time;
		ar & creator;
		ar & accuracy;
		ar & receive_at_ttl;
		ar & transaction_hash;
	}
};

class transaction_receipt : i_hashable, i_json_serialization
{
public:
	transaction_receipt_without_hash_sig content;
	std::string hash_sha256;
	std::string signature;
	
	void to_byte_buffer(byte_buffer& target) const
	{
		content.to_byte_buffer(target);
		target.add(hash_sha256);
		target.add(signature);
	}
	
	i_json_serialization::json to_json() const
	{
		i_json_serialization::json output;
		output["content"] = content.to_json();
		output["hash_sha256"] = hash_sha256;
		output["signature"] = signature;
		
		return output;
	}
	
	void from_json(const i_json_serialization::json& json_target)
	{
		content.from_json(json_target["content"]);
		hash_sha256 = json_target["hash_sha256"];
		signature = json_target["signature"];
		
	}
	
	bool operator==(const transaction_receipt& target) const
	{
		return (content == target.content) &&
				(hash_sha256 == target.hash_sha256) &&
				(signature == target.signature);
	}
	
	bool operator!=(const transaction_receipt& target) const
	{
		return !operator==(target);
	}

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & content;
		ar & hash_sha256;
		ar & signature;
	}
};

class transaction : i_hashable, i_json_serialization
{
public:
	transaction_without_hash_sig content;
	std::string hash_sha256;
	std::string signature;
	std::unordered_map<std::string, transaction_receipt> receipts;
	
	void to_byte_buffer(byte_buffer& target) const
	{
		content.to_byte_buffer(target);
		target.add(hash_sha256);
		target.add(signature);
		
		for (auto& iter: receipts)
		{
			target.add(iter.first);
			iter.second.to_byte_buffer(target);
		}
	}
	
	i_json_serialization::json to_json() const
	{
		i_json_serialization::json output;
		i_json_serialization::json content_json = content.to_json();
		output["content"] = content_json;
		output["hash_sha256"] = hash_sha256;
		output["signature"] = signature;
		
		//receipts
		i_json_serialization::json json_receipts = i_json_serialization::json::object();
		for (auto& [key,value]: receipts)
		{
			json_receipts[key] = value.to_json();
		}
		output["receipts"] = json_receipts;
		
		return output;
	}
	
	void from_json(const i_json_serialization::json& json_target)
	{
		content.from_json(json_target["content"]);
		hash_sha256 = json_target["hash_sha256"];
		signature = json_target["signature"];
		
		//receipts
		i_json_serialization::json json_receipts = json_target["receipts"];
		for (auto& [key,value]: json_receipts.items())
		{
			transaction_receipt temp;
			temp.from_json(value);
			receipts[key] = temp;
		}
		
	}
	
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & content;
		ar & hash_sha256;
		ar & signature;
		ar & receipts;
	}
};

class transaction_generator
{
public:
	transaction_generator() = default;
	
	void set_key(const crypto::hex_data& private_key, const crypto::hex_data& public_key, const crypto::hex_data& address)
	{
		_private_key = private_key;
		_public_key = public_key;
		_address = address;
		self_node_info.node_address = _address.getTextStr_lowercase();
		self_node_info.node_pubkey = _public_key.getTextStr_lowercase();
	}
	
	bool verify_key() const
	{
		//check private key and public key
		if (!dfl_util::verify_private_public_key(_private_key, _public_key)) return false;
		
		//check node_name & pubkey
		if (!dfl_util::verify_address_public_key(_address, _public_key)) return false;
		
		return true;
	}
	
	transaction generate(const std::string& parameter, const std::string& accuracy)
	{
		transaction output;
		output.content.creator = self_node_info;
		output.content.accuracy = accuracy;
		output.content.creation_time = time_util::get_current_utc_time();
		output.content.expire_time = output.content.creation_time + 60;
		output.content.ttl = 10;
		output.content.model_data = parameter;
		
		//hash
		byte_buffer output_buffer;
		output.content.to_byte_buffer(output_buffer);
		auto hash_hex = crypto::sha256::digest_s(output_buffer.data(), output_buffer.size());
		output.hash_sha256 = hash_hex.getTextStr_lowercase();
		
		//signature
		auto signature_hex = crypto::ecdsa_openssl::sign(hash_hex, _private_key);
		output.signature = signature_hex.getTextStr_lowercase();
		
		return output;
	}

	enum class append_receipt_status
	{
		unknown,
		success,
		ttl_expire
		
	};
	
	std::tuple<append_receipt_status, std::optional<transaction_receipt>> append_receipt(transaction& trans, const std::string& accuracy)
	{
		//ttl
		int current_ttl = trans.content.ttl;
		for (auto& [previous_receipt_hash, previous_receipt]: trans.receipts)
		{
			if (previous_receipt.content.receive_at_ttl < current_ttl)
			{
				current_ttl = previous_receipt.content.receive_at_ttl;
			}
		}
		if (current_ttl <= 0) return {append_receipt_status::ttl_expire, std::nullopt};
		
		transaction_receipt receipt;
		receipt.content.creation_time = time_util::get_current_utc_time();
		receipt.content.accuracy = accuracy;
		receipt.content.creator = self_node_info;
		receipt.content.transaction_hash = trans.hash_sha256;
		receipt.content.receive_at_ttl = current_ttl - 1;
		
		//hash
		byte_buffer output_buffer;
		receipt.content.to_byte_buffer(output_buffer);
		auto hash_hex = crypto::sha256::digest_s(output_buffer.data(), output_buffer.size());
		receipt.hash_sha256 = hash_hex.getTextStr_lowercase();
		
		//signature
		auto signature_hex = crypto::ecdsa_openssl::sign(hash_hex, _private_key);
		receipt.signature = signature_hex.getTextStr_lowercase();
		
		//append receipt
		trans.receipts[receipt.hash_sha256] = receipt;
		
		return {append_receipt_status::success, {receipt}};
	}

private:
	node_info self_node_info;
	crypto::hex_data _private_key;
	crypto::hex_data _public_key;
	crypto::hex_data _address;
};

class transaction_helper
{
public:
	static void save_to_file(const transaction& trans, const std::string& path)
	{
		auto json = trans.to_json();
		std::ofstream file(path, std::ios::binary);
		file << json.dump(4);
		file.flush();
		file.close();
	}
};