#include <crypto/sha256.hpp>
#include <crypto/md5.hpp>
#include <crypto/ecdsa_openssl.hpp>
#include <iostream>
#include <glog/logging.h>
#include <string>
int main()
{
    std::string message = "model1";
    crypto::sha256 sha256_test;
    auto output1 =sha256_test.digest(message);
    std::cout<<output1.getTextStr_lowercase()<<std::endl;
    std::string answer = "22ff0efe270b305371c97346e0619c987cfd10a5bb7c3acfcd7c92790a9ca91c";
    CHECK_EQ(output1.getTextStr_lowercase(), answer) << "pass faild.";

    std::string message1 = "BlockChain";
	auto output2 = sha256_test.digest(message1);
    std::cout<<output2.getTextStr_lowercase()<<std::endl;
    std::string answer2= "3a6fed5fc11392b3ee9f81caf017b48640d7458766a8eb0382899a605b41f2b9";
    CHECK_EQ(output2.getTextStr_lowercase(), answer2) << "pass faild.";


    std::string message2 = "BlockChain";
    crypto::md5 md5_test;
    auto output3 = md5_test.digest(message2);
    std::cout<<output3.getTextStr_lowercase()<<std::endl;
    std::string answer3= "8ddb53d8edaf2e25694a5d8abb852cd1";
    CHECK_EQ(output3.getTextStr_lowercase(), answer3) << "pass faild.";

    const std::string buff ="this is a ecdsa testing";
    crypto::ecdsa_openssl ecdsa_test;
    ecdsa_test.generate_key_pairs();
    ecdsa_test.sha256Hash(buff);
    auto prv = ecdsa_test.load_private_key();
    auto pub = ecdsa_test.load_public_key();
    unsigned char  *der;
    unsigned int der_len;
    ecdsa_test.sign(&der,&der_len);
    auto res = ecdsa_test.verify(der,der_len);

	return 0;


}
