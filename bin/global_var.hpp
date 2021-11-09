#pragma once

#include <crypto.hpp>
#include <ml_layer.hpp>

namespace global_var
{
	crypto::hex_data public_key("");
	crypto::hex_data private_key("");
	crypto::hex_data address("");
	int ml_test_batch_size;
	std::string ml_model_stream_type;
	float ml_model_stream_compressed_filter_limit;
	int estimated_transaction_per_block;
	bool enable_profiler;
}