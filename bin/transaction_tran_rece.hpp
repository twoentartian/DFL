#pragma once

#include <network.hpp>
#include <mutex>
#include <glog/logging.h>

#include <boost_serialization_wrapper.hpp>

#include "transaction.hpp"
#include "global_types.hpp"
#include "command_allocation.hpp"
#include "std_output.hpp"

class transaction_tran_rece
{
public:
	using receive_transaction_callback = std::function<void(const transaction& trans)>;
	
	transaction_tran_rece()
	{
	
	}

	void start_listen(uint16_t listen_port)
	{
		using namespace network;
		_p2p.start_service(listen_port);
		_p2p.set_receive_callback([this](header::COMMAND_TYPE command, const char *data, int length) -> std::tuple<header::COMMAND_TYPE, std::string> {
			if (command == command::transaction)
			{
				//this is a transaction
				std::stringstream ss;
				ss << std::string(data, length);
				transaction trans;
				try
				{
					trans = deserialize_wrap<boost::archive::binary_iarchive, transaction>(ss);
				}
				catch (...)
				{
					return {command::acknowledge_but_not_accepted, "cannot parse packet data"};
				}
				std::thread temp_thread([this, trans](){
					for (auto&& cb : _receive_transaction_callbacks)
					{
						cb(trans);
					}
				});
				temp_thread.detach();
				
				return {command::acknowledge, ""};
			}
			
			else
			{
				LOG(WARNING) << "[p2p] unknown command";
				return {command::unknown, ""};
			}
			
		});
	}
	
	void broadcast_transaction(const transaction& trans)
	{
		const std::string& trans_hash = trans.hash_sha256;
		
		std::string trans_binary_str = serialize_wrap<boost::archive::binary_oarchive>(trans).str();
		std::vector<peer_enpoint> peers_copy;
		{
			std::lock_guard guard(_peers_lock);
			peers_copy = _peers;
		}
		
		for (auto&& peer: peers_copy)
		{
			using namespace network;
			_p2p.send(peer.address, peer.port, i_p2p_node_with_header::ipv4, command::transaction, trans_binary_str.data(), trans_binary_str.length(), [trans_hash, peer](i_p2p_node_with_header::send_packet_status status, const char* data, int length){
				std::stringstream ss;
				ss << "send transaction with hash " << trans_hash << " to " << peer.to_string() << ", send status: " << i_p2p_node_with_header::send_packet_status_message[status];
				auto ss_str = ss.str();
				LOG(INFO) << ss_str;
				std_cout::println(ss_str);
			});
		}
	}
	
	void set_receive_transaction_callback(receive_transaction_callback callback)
	{
		_receive_transaction_callbacks.push_back(callback);
	}

	void add_peer(const std::string& address, uint16_t port)
	{
		peer_enpoint temp(address, port);
		{
			std::lock_guard guard(_peers_lock);
			_peers.push_back(temp);
		}
	}
	
private:
	std::vector<receive_transaction_callback> _receive_transaction_callbacks;
	network::p2p_with_header _p2p;
	std::vector<peer_enpoint> _peers;
	std::mutex _peers_lock;
};