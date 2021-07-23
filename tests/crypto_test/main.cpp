#include <crypto/sha256.hpp>
#include <crypto/md5.hpp>
#include <crypto/ecdsa_openssl.hpp>
#include <iostream>
#include <glog/logging.h>
#include <string>
#include "measure_time.hpp"
int main()
{
	//sha256
    std::string message = "model1";
    crypto::sha256 sha256_test;
    auto output1 =sha256_test.digest(message);
    std::cout<<output1.getTextStr_lowercase()<<std::endl;
    std::string answer = "22ff0efe270b305371c97346e0619c987cfd10a5bb7c3acfcd7c92790a9ca91c";
    CHECK_EQ(output1.getTextStr_lowercase(), answer) << "pass failed.";

	//sha256
    std::string message1 = "BlockChain";
	MEASURE_TIME(auto output2 = sha256_test.digest(message1);)
	MEASURE_TIME(auto output2_openssl = sha256_test.digest_openssl(message1);)
    std::cout<<output2.getTextStr_lowercase()<<std::endl;
    std::string answer2= "3a6fed5fc11392b3ee9f81caf017b48640d7458766a8eb0382899a605b41f2b9";
    CHECK_EQ(output2.getTextStr_lowercase(), answer2) << "pass failed.";
	CHECK_EQ(output2_openssl.getTextStr_lowercase(), answer2) << "pass failed.";

	//md5
    std::string message2 = "BlockChain";
    crypto::md5 md5_test;
	MEASURE_TIME(auto output3 = md5_test.digest(message2);)
	MEASURE_TIME(auto output3_openssl = md5_test.digest_openssl(message2);)
    std::cout<<output3.getTextStr_lowercase()<<std::endl;
    std::string answer3= "8ddb53d8edaf2e25694a5d8abb852cd1";
    CHECK_EQ(output3.getTextStr_lowercase(), answer3) << "pass failed.";
	CHECK_EQ(output3_openssl.getTextStr_lowercase(), answer3) << "pass failed.";

	//ecdsa
	const std::string buff ="this is a ecdsa testing";
	crypto::sha256 sha256;
	auto digest = sha256.digest(buff);
//	auto [pubKey, prvKey] = crypto::ecdsa_openssl::generate_key_pairs();
	auto keypair = crypto::ecdsa_openssl::generate_key_pairs();
	auto pubKey = std::get<0>(keypair);
    auto prvKey = std::get<1>(keypair);
	auto sig = crypto::ecdsa_openssl::sign(digest, prvKey);
	auto res1 = crypto::ecdsa_openssl::verify(sig, digest, pubKey);
	CHECK_EQ(res1, 1) << "pass faild.";

	sig.getHexMemory()[0]++;
	sig.update_text_from_hex();
	auto res2 = crypto::ecdsa_openssl::verify(sig, digest, pubKey);
	CHECK_EQ(res2, 0) << "pass failed.";


	return 0;
}
