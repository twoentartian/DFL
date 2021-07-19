#pragma once

#include <iostream>
#include <chrono>
#include <string>
#include <unordered_map>

#include <json_serialization.hpp>
#include <boost_serialization_wrapper.hpp>
#include <hash_interface.hpp>
#include <time_util.hpp>

#include "transaction.hpp"

class block_confirmation : i_hashable, i_json_serialization
{
public:
	std::string transaction_hash;
	std::string receipt_hash;
	std::string block_hash;
	
	std::string final_hash;
	node_info creator;
	std::string signature;
	
	void to_byte_buffer(byte_buffer& target) const
	{
		target.add(transaction_hash);
		target.add(receipt_hash);
		target.add(block_hash);
		creator.to_byte_buffer(target);
		
		//target.add(final_hash); //we comment this line because we are going to calculate the final hash
		//target.add(signature); //we comment this line because we are going to calculate the final hash
	}
	
	i_json_serialization::json to_json() const
	{
		i_json_serialization::json output;
		output["transaction_hash"] = transaction_hash;
		output["receipt_hash"] = receipt_hash;
		output["block_hash"] = block_hash;
		
		output["final_hash"] = final_hash;
		output["creator"] = creator.to_json();
		output["signature"] = signature;
		
		return output;
	}
	
	void from_json(const i_json_serialization::json& json_target)
	{
		transaction_hash = json_target["transaction_hash"];
		receipt_hash = json_target["receipt_hash"];
		block_hash = json_target["block_hash"];
		
		final_hash = json_target["final_hash"];
		creator.from_json(json_target["creator"]);
		signature = json_target["signature"];
	}

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & transaction_hash;
		ar & receipt_hash;
		ar & block_hash;
		
		ar & final_hash;
		ar & creator;
		ar & signature;
	}
};

class block_content : i_hashable, i_json_serialization
{
public:
	std::string previous_block_hash;
	node_info creator;
	std::unordered_map<std::string, transaction> transaction_container;
	TIME_STAMP_TYPE block_generated_time;
	
	std::string genesis_content;
	std::string genesis_block_hash;
	std::string memo;
	
	void to_byte_buffer(byte_buffer& target) const
	{
		target.add(previous_block_hash);
		creator.to_byte_buffer(target);
		for (auto& iter: transaction_container)
		{
			target.add(iter.first);
			iter.second.to_byte_buffer(target);
		}
		
		target.add(block_generated_time);
		target.add(genesis_content);
		target.add(genesis_block_hash);
		target.add(memo);
	}
	
	i_json_serialization::json to_json() const
	{
		i_json_serialization::json output;
		output["previous_block_hash"] = previous_block_hash;
		output["creator"] = creator.to_json();
		i_json_serialization::json transaction_container_json;
		for (auto& [key,value]: transaction_container)
		{
			transaction_container_json[key] = value.to_json();
		}
		output["transaction_container"] = transaction_container_json;
		output["block_generated_time"] = block_generated_time;
		
		if (!genesis_content.empty())
		{
			output["genesis_content"] = Base64::Encode(genesis_content);////genesis content is base64 encoded
		}
		else
		{
			output["genesis_content"] = "";
		}

		output["genesis_block_hash"] = genesis_block_hash;
		output["memo"] = memo;
		
		return output;
	}
	
	void from_json(const i_json_serialization::json& json_target)
	{
		previous_block_hash = json_target["previous_block_hash"];
		creator.from_json(json_target["creator"]);
		i_json_serialization::json transaction_container_json = json_target["transaction_container"];
		transaction_container.clear();
		for (auto& [key, value] : transaction_container_json.items())
		{
			transaction temp_trans;
			temp_trans.from_json(value);
			transaction_container[key] = temp_trans;
		}
		block_generated_time = json_target["block_generated_time"];
		std::string genesis_content_str = json_target["genesis_content"];
		if (genesis_content_str.empty())
		{
			genesis_content = "";
		}
		else
		{
			Base64::Decode(genesis_content_str, genesis_content);
		}
		genesis_block_hash = json_target["genesis_block_hash"];
		memo = json_target["memo"];
	}

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & previous_block_hash;
		ar & creator;
		ar & transaction_container;
		ar & block_generated_time;
		ar & genesis_content;
		ar & genesis_block_hash;
		ar & memo;
	}
};

class block : i_hashable, i_json_serialization
{
public:
	uint64_t height;
	block_content content;
	std::string block_content_hash;
	
	std::unordered_map<std::string, block_confirmation> block_confirmation_container;
	TIME_STAMP_TYPE block_finalization_time;
	std::string final_hash;
	
	void to_byte_buffer(byte_buffer& target) const
	{
		target.add(height);
		content.to_byte_buffer(target);
		target.add(block_content_hash);
		for (auto& [key,value]: block_confirmation_container)
		{
			target.add(key);
			value.to_byte_buffer(target);
		}
		
		target.add(block_finalization_time);
		//target.add(final_hash); // we ignore this final_hash because we are going to calculate it.
	}
	
	i_json_serialization::json to_json() const
	{
		i_json_serialization::json output;
		i_json_serialization::json block_confirmation_container_json;
		for (auto& [key, value] : block_confirmation_container)
		{
			block_confirmation_container_json[key] = value.to_json();
		}
		
		output["height"] = height;
		output["content"] = content.to_json();
		output["block_content_hash"] = block_content_hash;
		output["block_confirmation_container"] = block_confirmation_container_json;
		output["block_finalization_time"] = block_finalization_time;
		output["final_hash"] = final_hash;
		
		return output;
	}
	
	void from_json(const i_json_serialization::json& json_target)
	{
		height = json_target["height"];
		content.from_json(json_target["content"]);
		block_content_hash = json_target["block_content_hash"];
		block_confirmation_container.clear();
		for (auto& [key, value] : json_target["block_confirmation_container"].items())
		{
			block_confirmation temp_confirmation;
			temp_confirmation.from_json(value);
			block_confirmation_container[key] = temp_confirmation;
		}
		
		block_finalization_time = json_target["block_finalization_time"];
		final_hash = json_target["final_hash"];
	}

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & height;
		ar & content;
		ar & block_content_hash;
		ar & block_confirmation_container;
		ar & block_finalization_time;
		ar & final_hash;
	}
};
