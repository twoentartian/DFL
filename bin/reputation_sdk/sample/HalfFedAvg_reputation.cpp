#include <reputation_sdk.hpp>

template<typename DType>
class reputation_implementation : public reputation_interface<DType>
{
public:
	void update_model(Ml::caffe_parameter_net<DType> &current_model, double self_accuracy, const std::vector<updated_model<DType>> &models, std::unordered_map<std::string, double> &reputation) override
	{
		Ml::fed_avg_buffer<Ml::caffe_parameter_net<DType>> parameter_buffers(models.size());
		
		for (int i = 0; i < models.size(); i++)
		{
			parameter_buffers.add(models[i].model_parameter);
		}
		
		current_model = (current_model + parameter_buffers.average()) / 2;
	}
};

EXPORT_REPUTATION_API