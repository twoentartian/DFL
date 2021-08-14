//
// Created by gzr on 24-06-21.
//
#pragma once

#include <boost/crc.hpp>
#include <memory>
#include <string>

#include <functional>
#include <utility>

#include <glog/logging.h>
#include <byte_buffer.hpp>

namespace network
{
	struct header
	{
		using COMMAND_TYPE = uint16_t;
		
		uint32_t data_length;   //4 byte
		COMMAND_TYPE command_type;  //4 byte
		uint64_t reserved0;     //8 byte
		uint16_t crc;       //crc must be the last var in header
	};
	
	class packet_header
	{
	private:
		union header_union
		{
			header content;
			uint8_t raw_byte[sizeof(header)];
		};
		
		header_union header_part{};
		
	public:
		static constexpr int header_length = sizeof(header);
		
		packet_header()
		{
			for (unsigned char & i : header_part.raw_byte){i = 0;}
		}
		
		packet_header(header::COMMAND_TYPE type, size_t data_size) : packet_header()
		{
			header_part.content.command_type = type;//H.data_CRC = GetCrc16_data(data,data_size);
			header_part.content.data_length = data_size;
			header_part.content.crc = calculate_crc16_header();
		};
		
		packet_header(header::COMMAND_TYPE type, const std::string& data) : packet_header(type, data.length())
		{
		
		};
		
		packet_header(const uint8_t* raw_byte_data)
		{
			for (int i = 0; i < packet_header::header_length; ++i)
			{
				header_part.raw_byte[i] = raw_byte_data[i];
			}
		};
		
		header& get_header()
		{
			return header_part.content;
		}
		
		std::string get_header_byte()
		{
			std::string output(reinterpret_cast<const char*>(header_part.raw_byte), header_length);
			return output;
		}
		
		uint16_t calculate_crc16_header()
		{
			boost::crc_16_type result;
			byte_buffer buffer;
			buffer.add(header_part.content.data_length);
			buffer.add(header_part.content.command_type);
			buffer.add(header_part.content.reserved0);
			
			result.process_bytes(buffer.data(), buffer.size());
			return result.checksum();
		}
	};
	
	class header_decoder
	{
	public:
		using ReceivePacketCallback = std::function<void(header::COMMAND_TYPE command, std::shared_ptr<std::string>)>;

		header_decoder() : _packet_ongoing(false)
		{
		
		}
		
		void receive_data(const uint8_t* data, size_t length)
		{
			std::lock_guard guard(_lock);
			
			if (length < packet_header::header_length)
			{
				//it can only be an ongoing packet or error
				if (_packet_ongoing)
				{
					add_data_to_buffer(data, length);
				}
				else
				{
					LOG(WARNING) << "[network] receive an error packet, which is short than header length and not ongoing packet";
				}
				return;
			}
			
			//process header
			packet_header socket_header(data);
			uint16_t crc = socket_header.calculate_crc16_header();
			auto& header = socket_header.get_header();
			if (crc != header.crc)
			{
				if (_packet_ongoing)
				{
					add_data_to_buffer(data, length);
					return;
				}
				else
				{
					LOG(WARNING) << "[network] receive an packet header with wrong crc";
					return;
				}
			}
			if (_packet_ongoing)
			{
				//it might be a header, or an ongoing packet(crc == data by chance)
				if (length <= _remain_length)
				{
					//printf("new data packet arrived: %d\n", length);
					
					//an ongoing packet
					add_data_to_buffer(data, length);
					return;
				}
				else
				{
					//definitely new header
					LOG(WARNING) << "[network] packet ends in advance";
				}
			}
			
			//new packet header
			//printf("new packet header\n");
			_buffer.reset(new std::string());
			_current_command = header.command_type;
			_remain_length = header.data_length;
			
			const size_t payload_length = length - packet_header::header_length;
			const uint8_t* payload_loc = data + packet_header::header_length;
			add_data_to_buffer(payload_loc, payload_length);
		}
		
		void receive_data(const std::string& data)
		{
			receive_data(reinterpret_cast<const uint8_t*>(data.data()), data.length());
		}
		
		void receive_data(const char* data, size_t length)
		{
			receive_data(reinterpret_cast<const uint8_t*>(data), length);
		}
		
		void set_receive_callback(ReceivePacketCallback cb)
		{
			_receive_callback = std::move(cb);
		}
		
	private:
		ReceivePacketCallback _receive_callback;
		bool _packet_ongoing;
		std::shared_ptr<std::string> _buffer;
		size_t _remain_length;
		uint16_t _current_command;
		std::mutex _lock;
		
		void reset()
		{
			_buffer.reset();
			_current_command = 0;
			_packet_ongoing = false;
			_remain_length = 0;
		}
		
		void add_data_to_buffer(const uint8_t* data, size_t length)
		{
			_buffer->append(reinterpret_cast<const char*>(data), length);
			size_t previous_remain_length = _remain_length;
			_remain_length = _remain_length - length;
			if (_remain_length > previous_remain_length)
			{
				LOG(WARNING) << "[network] packet length overflow";
				reset();
				return;
			}
			if (_remain_length == 0)
			{
				//printf("packet end\n");
				if (_receive_callback) _receive_callback(_current_command, _buffer);
				reset();
				return;
			}
			else
			{
				//printf("packet continue, remain length: %d\n", _remain_length);
				_packet_ongoing = true;
			}
		}
		
	};
	
}

