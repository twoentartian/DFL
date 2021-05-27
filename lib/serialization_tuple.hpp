#pragma once

namespace boost
{
	namespace serialization
	{
		template <unsigned N>
		struct Serialize
		{
			template <class Archive, typename... Args>
			static void serialize(Archive &ar, std::tuple<Args ...> &t, unsigned int version)
			{
				ar &std::get<N - 1>(t);
				Serialize<N - 1>::serialize(ar, t, version);
			}
		};
		
		template <>
		struct Serialize<0>
		{
			template <class Archive, typename... Args>
			static void serialize(Archive &ar, std::tuple<Args ...> &t, unsigned int version)
			{
			
			}
		};
		
		template <class Archive, typename... Args>
		void serialize(Archive &ar, std::tuple<Args ...> &t, unsigned int version)
		{
			Serialize<sizeof... (Args)>::serialize(ar, t, version);
		}
	}
}