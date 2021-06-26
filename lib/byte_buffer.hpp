#pragma once

#include <iostream>

class byte_buffer
{
public:
	static constexpr size_t RESERVED_SPACE = 10000;
	
	byte_buffer() : byte_buffer(RESERVED_SPACE){}
	
	explicit byte_buffer(size_t size)
	{
		_buffer = new uint8_t[size];
		_loc = 0;
		_capacity = size;
	}
	
	byte_buffer(const byte_buffer& target) = delete;
	
	void swap(byte_buffer& target)
	{
		uint8_t *temp_ptr = this->_buffer;
		this->_buffer = target._buffer;
		target._buffer = temp_ptr;
	}
	
	~byte_buffer()
	{
		delete[] _buffer;
	}
	
	[[nodiscard]] uint8_t* data() const
	{
		return _buffer;
	}
	
	[[nodiscard]] size_t size() const
	{
		return _loc;
	}
	
	inline void add(const byte_buffer& value)
	{
		for (int i = 0; i < value._loc; i++)
		{
			_buffer[_loc] = value.data()[i];
			++_loc;
			if (_loc == _capacity)
			{
				ExtendCapacity();
			}
		}
	}
	
	inline void add(const uint8_t& value)
	{
		_buffer[_loc] = value;
		++_loc;
		if (_loc == _capacity)
		{
			ExtendCapacity();
		}
	}
	
	inline void add(const std::string& value)
	{
		for (auto& character : value)
		{
			_buffer[_loc] = static_cast<uint8_t>(character);
			++_loc;
			if (_loc == _capacity)
			{
				ExtendCapacity();
			}
		}
	}
	
	inline void add(const uint8_t* values, const uint32_t& length)
	{
		for (uint32_t i = 0; i < length; i++)
		{
			_buffer[_loc] = values[i];
			++_loc;
			if (_loc == _capacity)
			{
				ExtendCapacity();
			}
		}
	}
	
	inline void add(uint32_t value)
	{
		for (uint32_t i = 0; i < 4; i++)
		{
			_buffer[_loc] = static_cast<uint8_t>((value >> 8 * i) & 0xff);
			++_loc;
			if (_loc == _capacity)
			{
				ExtendCapacity();
			}
		}
	}
    inline void add(uint16_t value)
    {
        for (uint32_t i = 0; i < 2; i++)
        {
            _buffer[_loc] = static_cast<uint8_t>((value >> 8 * i) & 0xff);
            ++_loc;
            if (_loc == _capacity)
            {
                ExtendCapacity();
            }
        }
    }
	inline void add(int32_t value)
	{
		for (uint32_t i = 0; i < 4; i++)
		{
			_buffer[_loc] = static_cast<uint8_t>((value >> 8 * i) & 0xff);
			++_loc;
			if (_loc == _capacity)
			{
				ExtendCapacity();
			}
		}
	}
	
	inline void add(int64_t value)
	{
		for (uint32_t i = 0; i < 8; i++)
		{
			_buffer[_loc] = static_cast<uint8_t>((value >> 8 * i) & 0xff);
			++_loc;
			if (_loc == _capacity)
			{
				ExtendCapacity();
			}
		}
	}
	
	inline void add(uint64_t value)
	{
		for (uint32_t i = 0; i < 8; i++)
		{
			_buffer[_loc] = static_cast<uint8_t>((value >> 8 * i) & 0xff);
			++_loc;
			if (_loc == _capacity)
			{
				ExtendCapacity();
			}
		}
	}
	
	inline void add(bool value)
	{
		_buffer[_loc] = static_cast<uint8_t>(value);
		++_loc;
		if (_loc == _capacity)
		{
			ExtendCapacity();
		}
	}
	
private:
	uint8_t* _buffer;
	size_t _capacity;
	size_t _loc;
	
	void ExtendCapacity()
	{
		uint8_t* oldBuffer = _buffer;
		size_t new_size = _capacity*2;
		_buffer = new uint8_t[new_size];
		for (uint32_t i = 0; i < _capacity; ++i)
		{
			_buffer[i] = oldBuffer[i];
		}
		delete[] oldBuffer;
		_capacity = new_size;
	}
};
