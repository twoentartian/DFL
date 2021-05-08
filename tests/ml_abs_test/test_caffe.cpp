//
// Created by tyd.
//

#include <caffe/util/io.hpp>
#include <ml_layer/ml_abs.hpp>
#include <ml_layer/caffe.hpp>
#include <caffe/sgd_solvers.hpp>
#include <lmdb.h>

#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_SUITE (ml_caffe_test)

BOOST_AUTO_TEST_CASE (structure_same_test)
{
	Ml::MlCaffeModel<float> model1;
	model1.load_caffe_model<caffe::SGDSolver>("../../../dataset/MNIST/lenet_solver.prototxt");
	Ml::MlCaffeModel<float> model2 = model1;
	
	
	Ml::caffe_parameter_net<float> net1 = model1.get_parameter();
	for (int i = 0; i < net1.getLayers().size(); ++i)
	{
		auto& data = net1.getLayers()[i].getBlob_p().get()->getData();
		for (auto&& s : data)
		{
			s = 0;
		}
	}
	model1.set_parameter(net1);
	std::string structure1 = model1.get_network_structure_info();
	std::string structure2 = model2.get_network_structure_info();

	BOOST_CHECK(structure1 == structure2);
}

BOOST_AUTO_TEST_CASE (get_model_parameter)
{
	Ml::MlCaffeModel<float> model1;
	model1.load_caffe_model<caffe::SGDSolver>("../../../dataset/MNIST/lenet_solver.prototxt");
	Ml::MlCaffeModel<float> model2 = model1;
	Ml::caffe_parameter_net<float> parameter1 = model1.get_parameter();
	for (int i = 0; i < parameter1.getLayers().size(); ++i)
	{
		auto& data = parameter1.getLayers()[i].getBlob_p()->getData();
		for (auto&& s : data)
		{
			s = 0;
		}
	}
	model1.set_parameter(parameter1);
	auto parameter2 = model1.get_parameter();
	bool pass = true;
	for (int i = 0; i < parameter1.getLayers().size(); ++i)
	{
		auto& data1 = parameter1.getLayers()[i].getBlob_p()->getData();
		auto& data2 = parameter2.getLayers()[i].getBlob_p()->getData();
		if(data1 != data2) pass = false;
		break;
	}
	BOOST_CHECK(pass);
}

BOOST_AUTO_TEST_CASE (serialization)
{
	Ml::MlCaffeModel<float> model1;
	model1.load_caffe_model<caffe::SGDSolver>("../../../dataset/MNIST/lenet_solver.prototxt");
	Ml::MlCaffeModel<float> model2;
	model2.load_caffe_model<caffe::SGDSolver>("../../../dataset/MNIST/lenet_solver.prototxt");
	
	Ml::caffe_parameter_net<float> parameter1 = model1.get_parameter();
	for (int i = 0; i < parameter1.getLayers().size(); ++i)
	{
		auto& data = parameter1.getLayers()[i].getBlob_p()->getData();
		for (auto&& s : data)
		{
			s = 0;
		}
	}
	model1.set_parameter(parameter1);
	
	auto ss = model1.serialization(Ml::model_serialization_boost_binary);
	model2.deserialization(Ml::model_serialization_boost_binary,ss);
	auto parameter2 = model2.get_parameter();
	bool pass = true;
	for (int i = 0; i < parameter1.getLayers().size(); ++i)
	{
		auto& data1 = parameter1.getLayers()[i].getBlob_p()->getData();
		auto& data2 = parameter2.getLayers()[i].getBlob_p()->getData();
		if(data1 != data2) pass = false;
		break;
	}
	BOOST_CHECK(pass);
}

BOOST_AUTO_TEST_SUITE_END( )
