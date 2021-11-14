#pragma once

#include <unordered_map>
#include <functional>
#include <vector>
#include <random>

#include <network.hpp>
#include <boost_serialization_wrapper.hpp>

#include "../command_allocation.hpp"
#include "../global_types.hpp"
#include "../dfl_util.hpp"

class introducer_p2p
{
public:
	using new_peer_callback = std::function<void(const peer_endpoint& /*peer*/)>;
	
	introducer_p2p(const std::string& address, const std::string& privateKey, const std::string& publicKey)
	{
		_address.assign(address);
		_private_key.assign(privateKey);
		_public_key.assign(publicKey);
	}
	
	introducer_p2p(const crypto::hex_data& address, const crypto::hex_data& privateKey, const crypto::hex_data& publicKey)
	{
		_address = address;
		_private_key = privateKey;
		_public_key = publicKey;
	}
	
	void start_listen(uint16_t listen_port)
	{
		using namespace network;
		_p2p.start_service(listen_port);
		_p2p.set_receive_callback([this](header::COMMAND_TYPE command, const char *data, int length, std::string ip) -> std::tuple<header::COMMAND_TYPE, std::string>
		                          {
			                          if (command == command::register_as_peer)
			                          {
				                          //this is a register request
				                          std::stringstream ss;
				                          ss << std::string(data, length);
				                          register_as_peer_data register_request;
				                          try
				                          {
					                          register_request = deserialize_wrap<boost::archive::binary_iarchive, register_as_peer_data>(ss);
				                          }
				                          catch (...)
				                          {
					                          LOG(WARNING) << "cannot parse packet data";
					                          return {command::acknowledge_but_not_accepted, "cannot parse packet data"};
				                          }
				
				                          //verify signature
				                          if (!dfl_util::verify_signature(register_request.node_pubkey, register_request.signature, register_request.hash))
				                          {
					                          LOG(WARNING) << "verify signature failed";
					                          return {command::acknowledge_but_not_accepted, "cannot verify signature"};
				                          }
				
				                          //verify hash
				                          if (!dfl_util::verify_hash(register_request, register_request.hash))
				                          {
					                          LOG(WARNING) << "verify hash failed";
					                          return {command::acknowledge_but_not_accepted, "cannot verify hash"};
				                          }
				                          
				                          //verify node public key and address
				                          if (!dfl_util::verify_address_public_key(register_request.address, register_request.node_pubkey))
				                          {
					                          LOG(WARNING) << "verify node public key failed";
					                          return {command::acknowledge_but_not_accepted, "cannot verify node public key / address"};
				                          }
				                          
				                          //add to peer list
				                          auto[state, msg] = add_peer(register_request.address, register_request.node_pubkey, ip, register_request.port);
				                          if (!state)
				                          {
					                          LOG(WARNING) << "cannot add peer: " << msg;
					                          return {command::acknowledge_but_not_accepted, "cannot add peer: " + msg};
				                          }
				
				                          return {command::acknowledge, ""};
			                          }
			                          else if (command == command::request_peer_info)
			                          {
				                          //request for peer info
				                          std::stringstream ss;
				                          ss << std::string(data, length);
				                          request_peer_info_data peer_request;
				                          try
				                          {
					                          peer_request = deserialize_wrap<boost::archive::binary_iarchive, request_peer_info_data>(ss);
				                          }
				                          catch (...)
				                          {
					                          LOG(WARNING) << "cannot parse packet data";
					                          return {command::acknowledge_but_not_accepted, "cannot parse packet data"};
				                          }
				
				                          if (peer_request.requester_address != peer_request.generator_address)
				                          {
					                          return {command::acknowledge_but_not_accepted, "invalid requester and generator address"};
				                          }
				
				                          //verify signature
				                          if (!dfl_util::verify_signature(peer_request.node_pubkey, peer_request.signature, peer_request.hash))
				                          {
					                          LOG(WARNING) << "verify signature failed";
					                          return {command::acknowledge_but_not_accepted, "cannot verify signature"};
				                          }
				
				                          //verify hash
				                          if (!dfl_util::verify_hash(peer_request, peer_request.hash))
				                          {
					                          LOG(WARNING) << "verify hash failed";
					                          return {command::acknowledge_but_not_accepted, "cannot verify hash"};
				                          }
				
				                          //verify node public key and address
				                          if (!dfl_util::verify_address_public_key(peer_request.generator_address, peer_request.node_pubkey))
				                          {
					                          LOG(WARNING) << "verify node public key failed";
					                          return {command::acknowledge_but_not_accepted, "cannot verify node public key / address"};
				                          }
				                          
				                          //add peer
				                          std::unordered_map<std::string, peer_endpoint> peers_copy;
				                          {
				                          	  std::lock_guard guard(_peers_lock);
					                          peers_copy = _peers;
				                          }
				                          for (auto& [peer_name, peer] : peers_copy)
				                          {
					                          peer_request.peers_info.push_back(peer);
				                          }
				                          
				                          //shuffle peer list
				                          {
					                          std::random_device rd;
					                          std::mt19937 g(rd());
					                          std::shuffle(peer_request.peers_info.begin(), peer_request.peers_info.end(), g);
				                          }
				
				                          peer_request.generator_address = _address.getTextStr_lowercase();
				                          peer_request.node_pubkey = _public_key.getTextStr_lowercase();
				                          peer_request.hash = crypto::sha256_digest(peer_request).getTextStr_lowercase();
				                          auto sig_hex = crypto::ecdsa_openssl::sign(peer_request.hash, _private_key);
				                          peer_request.signature = sig_hex.getTextStr_lowercase();
				                          
				                          std::string reply = serialize_wrap<boost::archive::binary_oarchive>(peer_request).str();
				                          
				                          return {command::reply_peer_info, reply};
			                          }
			
			                          else
			                          {
				                          LOG(WARNING) << "[p2p] unknown command";
				                          return {command::unknown, ""};
			                          }
		                          });
		
	}
	
	//name: hash{public key}, address: network address.
	std::tuple<bool, std::string> add_peer(const std::string &name, const std::string &public_key, const std::string &address, uint16_t port)
	{
		auto iter = _peers.find(name);
		if (iter != _peers.end())
		{
			bool skip = true;
			if (public_key != iter->second.public_key) skip = false;
			if (address != iter->second.address) skip = false;
			if (port != iter->second.port) skip = false;
			if (skip) return {true, "already exist"};
		}
		
		peer_endpoint temp(name, public_key, address, port, peer_endpoint::peer_type_normal_node);
		{
			std::lock_guard guard(_peers_lock);
			_peers.emplace(name, temp);
		}
		
		for (auto& cb : _new_peer_callbacks)
		{
			cb(temp);
		}
		
		return {true, ""};
	}
	
	void add_new_peer_callback(const new_peer_callback& cb)
	{
		_new_peer_callbacks.push_back(cb);
	}
	
	[[nodiscard]] std::unordered_map<std::string, peer_endpoint> get_peers()
	{
		return _peers;
	}

private:
	network::p2p_with_header _p2p;
	
	std::unordered_map<std::string, peer_endpoint> _peers;
	std::mutex _peers_lock;
	
	crypto::hex_data _address;
	crypto::hex_data _public_key;
	crypto::hex_data _private_key;
	
	std::vector<new_peer_callback> _new_peer_callbacks;
};
