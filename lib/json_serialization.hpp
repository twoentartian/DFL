#pragma once

#include <nlohmann/json.hpp>

class i_json_serialization
{
public:
	using json = nlohmann::json;

	virtual json to_json() const = 0;
	
	virtual void from_json(const json& json_target) = 0;
};