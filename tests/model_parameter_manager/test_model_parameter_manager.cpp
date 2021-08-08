//
// Created by jxzhang on 26-07-21.
//

#include "../../bin/model_parameter_manager.hpp"

std::shared_ptr<model_parameter_manager> main_model_parameter_manager;


int main()
{
    Ml::MlCaffeModel<float,caffe::SGDSolver> model1;
    model1.load_caffe_model("../../../dataset/MNIST/lenet_solver_memory.prototxt");

    Ml::caffe_parameter_net<float> parameter_1=model1.get_parameter();
    Ml::caffe_parameter_net<float> parameter_stored=model1.get_parameter();
    std::optional<Ml::caffe_parameter_net<float>> parameter_got;

    main_model_parameter_manager.reset(new model_parameter_manager("./model_db"));
    main_model_parameter_manager->reset_model_parameter_database();

    sleep(1);

    std::vector<Ml::caffe_parameter_net<float>> parameter_stored_container;

//    single node test

    bool pass=true;

    std::string node_hash_1="123";

    parameter_stored_container.push_back(parameter_stored);
    main_model_parameter_manager->update_model_parameter_net<float>(node_hash_1,parameter_stored);

    for (int i=0;i<=10;i++)
    {
        parameter_stored=parameter_stored+parameter_1;
        parameter_got=main_model_parameter_manager->get_model_parameter_net_newest<float>(node_hash_1);

        if (parameter_stored==parameter_got.value())
        {
            pass=false;
            std::cout<<"failed single node test: "<<i<<std::endl;
            break;
        }

        parameter_stored_container.push_back(parameter_stored);
        main_model_parameter_manager->update_model_parameter_net<float>(node_hash_1,parameter_stored);
        parameter_got=main_model_parameter_manager->get_model_parameter_net_newest<float>(node_hash_1);

        if (parameter_stored!=parameter_got.value())
        {
            pass=false;
            std::cout<<"failed single node test: "<<i<<std::endl;
            break;
        }

        sleep(1);
    }

    if(pass)
    {
        std::cout<<"pass single node test"<<std::endl;
    }


//    single node get history test

    pass=true;

    auto parameter_got_history=main_model_parameter_manager->get_model_parameter_net_history<float>(node_hash_1);

    for (int i=0;i<parameter_got_history.value().size();i++)
    {
        auto temp_model=(parameter_got_history.value())[i].second;

        if (temp_model!=parameter_stored_container[i])
        {
            pass=false;
            std::cout<<"failed single node get history test: "<<i<<" "<< std::to_string((parameter_got_history.value())[i].first)<<std::endl;
            break;
        }

    }

    if (pass)
    {
        std::cout<<"pass single node get history test"<<std::endl;
    }


//    reset function test 1: single

    pass=true;

    main_model_parameter_manager->reset_model_parameter_database();

    parameter_got_history=main_model_parameter_manager->get_model_parameter_net_history<float>(node_hash_1);

    if (parameter_got_history)
    {
        pass=false;
        std::cout<<"failed reset function test on single node: not empty"<<std::endl;
    }
    else
    {
        std::cout<<"pass reset function test on single node"<<std::endl;
    }


//   reset function test 2: multiple nodes

    pass=true;

    std::string node_hash_2="456";
    parameter_stored=model1.get_parameter();

    for (int i=0;i<=10;i++) {
        parameter_stored = parameter_stored + parameter_1;
        main_model_parameter_manager->update_model_parameter_net<float>(node_hash_1, parameter_stored);
        main_model_parameter_manager->update_model_parameter_net<float>(node_hash_2,parameter_stored);
        sleep(1);
    }

    main_model_parameter_manager->reset_model_parameter_database(node_hash_1);

    parameter_got_history=main_model_parameter_manager->get_model_parameter_net_history<float>(node_hash_1);

    if (parameter_got_history)
    {
        pass=false;
        std::cout<<"failed reset function test on multiple nodes: not empty"<<std::endl;
    }
    else
    {
        parameter_got_history=main_model_parameter_manager->get_model_parameter_net_history<float>(node_hash_2);

        if (parameter_got_history.value().size()==11)
        {
            std::cout<<"pass reset function test on multiple node"<<std::endl;
        }
        else
        {
            pass=false;
            std::cout<<"failed reset function test on multiple nodes: wrongly delete"<<std::endl;
        }
    }

    main_model_parameter_manager->reset_model_parameter_database();

//     reset function test 3: reset the whole database\

    pass=true;

    parameter_stored=model1.get_parameter();

    for (int i=0;i<=10;i++) {
        parameter_stored = parameter_stored + parameter_1;
        main_model_parameter_manager->update_model_parameter_net<float>(node_hash_1, parameter_stored);
        main_model_parameter_manager->update_model_parameter_net<float>(node_hash_2,parameter_stored);
        sleep(1);
    }

    main_model_parameter_manager->reset_model_parameter_database();

    parameter_got_history=main_model_parameter_manager->get_model_parameter_net_history<float>(node_hash_1);

    if (parameter_got_history)
    {
        pass=false;
        std::cout<<"failed reset all database function test on multiple nodes: not empty: "<<node_hash_1<<std::endl;
    }

    parameter_got_history=main_model_parameter_manager->get_model_parameter_net_history<float>(node_hash_2);

    if (parameter_got_history)
    {
        pass=false;
        std::cout<<"failed reset all database function test on multiple nodes: not empty: "<<node_hash_2<<std::endl;
    }

    if (pass=true)
    {
        std::cout<<"pass reset all database function test on multiple nodes: "<<std::endl;
    }


}
