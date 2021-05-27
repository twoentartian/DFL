//
// Created by gzr on 26-05-21.
//

#pragma once


#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/ecdsa.h>
#include <openssl/sha.h>
#include "sha256.hpp"
// use secp256k1 elliptic curve
#define EC_CURVE NID_secp256k1

using namespace std;
namespace crypto {
    class ecdsa_openssl {
    public:
        void generate_key_pairs()
        {
            ec_key = EC_KEY_new();
            EC_GROUP *ecgroup_new = EC_GROUP_new_by_curve_name(EC_CURVE);
            const EC_GROUP *ec_group = ecgroup_new;
            if (!EC_KEY_set_group(ec_key, ec_group)) {
                printf("Initial EC_KEY_set_group failed");
            }
            if (!EC_KEY_generate_key(ec_key)) {
                printf("generate EC_KEY_generate_key failed");
            }
            BIGNUM *prv_;
            BIGNUM *pub_;
            prv_ = (BIGNUM *) EC_KEY_get0_private_key(ec_key);
            pub_ = (BIGNUM *) EC_KEY_get0_public_key(ec_key);
            prv = BN_bn2hex(prv_);
            pub = BN_bn2hex(pub_);

        }
        const char *load_private_key()
        {
            return prv;

        }

        const char *load_public_key()
        {
            return pub;
        }

        const string get_hash()
        {
            return digest_;
        }

        void sign(unsigned char **der, unsigned int*dlen)
        {
            ECDSA_SIG *signature;
            unsigned char *der_copy;
            signature = ECDSA_do_sign(reinterpret_cast<const unsigned char *>(&digest_), get_hash().size(), ec_key);
            *dlen = ECDSA_size(ec_key);
            *der = (unsigned char*)calloc(*dlen, sizeof(unsigned char));
            der_copy = *der;
            i2d_ECDSA_SIG(signature, &der_copy);
            ECDSA_SIG_free(signature);
        }
        bool verify(const unsigned char *der, unsigned int der_len)

        {
            const unsigned char* der_copy;
            ECDSA_SIG *signature;
            int res;
            der_copy = der;
            signature = d2i_ECDSA_SIG(NULL, &der_copy, der_len);
            /** 1:verified. 2:not verified. 3:library error. **/
            res = ECDSA_do_verify(reinterpret_cast<const unsigned char *>(&digest_), get_hash().size(), signature, ec_key);
            if(res==1)
            {
                printf("Verify Successfully");
                return true;
            }
            else
            {
                printf("Verify fails");
                return false;
            }
        }


        void sha256Hash(const string message) {
            crypto::sha256 checksum;
            auto hash = checksum.digest(message);
            digest_ = hash.getTextStr_lowercase();
        }


    private:
        EC_KEY *ec_key;
        char *prv;
        char *pub;
        string digest_;


    };




}