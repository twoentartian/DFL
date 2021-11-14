#define CPU_ONLY
#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>
#include <filesystem>

#include <glog/logging.h>

#include "../../bin/DFL_introducer/DFL_introducer_p2p.hpp"
#include "../../bin/transaction_tran_rece.hpp"

BOOST_AUTO_TEST_SUITE (introducer_test)

BOOST_AUTO_TEST_CASE (introducer_logic_process)
{
	std::shared_ptr<introducer_p2p> test_introducer_server;
	std::shared_ptr<transaction_tran_rece> test_dfl_node;
	std::shared_ptr<transaction_tran_rece> test_dfl_node2;
	
	uint16_t introducer_port = 7000;
	uint16_t dfl_node_port = 7001;
	uint16_t dfl_node2_port = 7002;
	
	//introducer
	std::string introducer_public_key = "077bcebba32c4377a6309e9d1cb165b7dc35788eb0bb187a3f61dcb9a3069cc17a2b601839142aff90aed1b9a9a5fb8fcc2655b51c91369c98c3faaa2be284d351";
	std::string introducer_private_key = "2bf5e439eeb3ba6253f118dfa8b3286809ec1748e0d702ea23f5922649e90af7";
	std::string introducer_address = "890d8d9ab1037968a87e52d62674256147400c4ab23ef3f8478d79fcb80481f4";
	
	test_introducer_server.reset(new introducer_p2p(introducer_public_key, introducer_private_key, introducer_address));
	test_introducer_server->start_listen(introducer_port);
	test_introducer_server->add_new_peer_callback([](const peer_endpoint& peer){
		std::cout << "new peer: " << peer.name << " - " << peer.address << ":" << peer.port << std::endl;
	});
	
	std::cout << "introducer starts at port " << introducer_port << std::endl;
	
	//DFL node
	std::string node_public_key = "06c1e9aa35cd31609f9865a39c2843f461f2bbcae43b4c4e2f4f5e941f69f17a5342b95b9c378e87be294b93a53e6b7bc79f88ed2a51b03a70d5fb2d95288ee628";
	std::string node_private_key = "8e0d0eb0fcfb570eef9039a4a5c08f8127914b37cfb942855aab836b8a206dd7";
	std::string node_address = "eb63cc0997108d628e04b62f1b1063baa3d7a8a6634d18568da66617fb35744e";
	
	test_dfl_node.reset(new transaction_tran_rece(node_public_key, node_private_key, node_address, nullptr));
	test_dfl_node->start_listen(dfl_node_port);
	test_dfl_node->add_introducer(introducer_address, "127.0.0.1", introducer_public_key, introducer_port);
	
	std::cout << "wait for 1 second" << std::endl;
	std::this_thread::sleep_for(std::chrono::seconds(1));
	{
		auto peers = test_introducer_server->get_peers();
		std::cout << "introducer peer count: " << peers.size() << std::endl;
		BOOST_CHECK(peers.size() == 1);
	}
	{
		auto peers = test_dfl_node->get_peers();
		std::cout << "node1 peer count: " << peers.size() << std::endl;
		BOOST_CHECK(peers.size() == 1);
	}

	
	//A new DFL node
	std::string node2_public_key = "07b4e3ca7e746c8fdf81c3e6ed768f69212c9f60c8202624e15e032b564bfb113f6587f28916234c65c406d8bc167b2c7965ddcb66390fe9afce92c844a03bffa5";
	std::string node2_private_key = "47a028b80b90747a57f0c7069ed3deb207531af653be6e079769c183c2283356";
	std::string node2_address = "073d3e2e9ccb5e4e85c92fb5ef0d0ceeab7f6ccb706e5b8396d3e8577e5f2376";
	
	test_dfl_node2.reset(new transaction_tran_rece(node2_public_key, node2_private_key, node2_address, nullptr));
	test_dfl_node2->start_listen(dfl_node2_port);
	test_dfl_node2->add_introducer(introducer_address, "127.0.0.1", introducer_public_key, introducer_port);
	test_dfl_node2->try_to_add_peer(5);
	
	std::this_thread::sleep_for(std::chrono::seconds(1));
	{
		auto peers = test_introducer_server->get_peers();
		std::cout << "introducer peer count: " << peers.size() << std::endl;
		BOOST_CHECK(peers.size() == 2);
	}
	{
		auto peers = test_dfl_node->get_peers();
		std::cout << "node1 peer count: " << peers.size() << std::endl;
		BOOST_CHECK(peers.size() == 2);
	}
	{
		auto peers = test_dfl_node2->get_peers();
		std::cout << "node2 peer count: " << peers.size() << std::endl;
		BOOST_CHECK(peers.size() == 1);
	}
	
}

BOOST_AUTO_TEST_SUITE_END()