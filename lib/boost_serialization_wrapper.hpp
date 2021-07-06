#pragma once

#include <sstream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/unordered_map.hpp>

#include "serialization_tuple.hpp"


template<class archive, class T>
std::stringstream serialize_wrap(const T& target)
{
	std::stringstream output;
	static_assert(std::is_same_v<archive, boost::archive::text_oarchive> || std::is_same_v<archive, boost::archive::binary_oarchive>);
	{
		archive oa(output);
		oa << target;
	}
	return output;
}

template<class archive, class T>
T deserialize_wrap(std::stringstream& stream)
{
	T output;
	static_assert(std::is_same_v<archive, boost::archive::text_iarchive> || std::is_same_v<archive, boost::archive::binary_iarchive>);
	{
		archive ia(stream);
		ia >> output;
	}
	return output;
}

template<class archive, class T>
T deserialize_wrap(const std::string& stream)
{
	std::stringstream ss;
	ss << stream;
	return deserialize_wrap<archive, T>(ss);
}
