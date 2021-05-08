//
// Created by tyd.
//

#include <caffe/util/io.hpp>

#include <ml_layer/ml_abs.hpp>
#include <ml_layer/caffe.hpp>
#include <caffe/sgd_solvers.hpp>
int main()
{
	caffe::Caffe::set_mode(caffe::Caffe::CPU);
	const std::string filename{ "../../../dataset/MNIST/lenet_solver.prototxt" };
	caffe::SolverParameter solver_param;
	if (!caffe::ReadProtoFromTextFile(filename.c_str(), &solver_param)) {
		fprintf(stderr, "parse solver.prototxt fail\n");
		return -1;
	}

	caffe::SGDSolver<float> solver(solver_param);
	caffe::NetParameter net_param;


	solver.net()->ToProto(&net_param);
	WriteProtoToBinaryFile(net_param, "solver.bin");
	WriteProtoToTextFile(net_param, "solver.txt");

	//solver.Step(1);


	const auto& layers = solver.net()->layers();
	int i = 0;
	for (auto&& single_layer : layers)
	{
		std::cout << single_layer->layer_param().type() << " " << single_layer->layer_param().name() << std::endl;
		if (!single_layer->blobs().empty())
		{
			std::cout << " " << single_layer->blobs()[0]->count() << std::endl;
		}

		caffe::LayerParameter layer_param;
		single_layer->ToProto(&layer_param);
		WriteProtoToBinaryFile(layer_param, std::to_string(i) + "layer.bin");
		WriteProtoToTextFile(layer_param, std::to_string(i) + "layer.txt");
		i++;
	}
	
	
	
}