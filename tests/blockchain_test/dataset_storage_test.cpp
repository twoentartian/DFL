#include "../../bin/dataset_storage.hpp"

int main()
{
	dataset_storage<float> storage("./test_db", 10);
	storage.set_full_callback([](const std::vector<Ml::tensor_blob_like<float>>&data, const std::vector<Ml::tensor_blob_like<float>>&label){
		std::cout << "get full callback, size: " << data.size() << std::endl;
	});
//	std::vector<Ml::tensor_blob_like<float>> data;
//	std::vector<Ml::tensor_blob_like<float>> label;
//	for (int i = 0; i < 50; ++i)
//	{
//		Ml::tensor_blob_like<float> single_data, single_label;
//		single_data.getShape() = {1};
//		single_data.getData() = {float (i)};
//		single_label.getShape() = {1};
//		single_label.getData() = {float (i%10)};
//		data.push_back(single_data);
//		label.push_back(single_label);
//	}
//	storage.add_data(data, label);
	storage.start_network_service(8040, 2);
	
	std::cout << "press any key to exit" << std::endl;
	std::cin.get();
	
	return 0;
}