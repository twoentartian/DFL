#pragma once

#include <tuple>
#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/ecdsa.h>
#include <openssl/sha.h>
#include "sha256.hpp"
#include "hex_data.hpp"

namespace crypto
{
	class ecdsa_openssl
	{
	public:
		enum curve_type
		{
			secp112r1 = NID_secp112r1,
			secp112r2 = NID_secp112r2,
			secp128r1 = NID_secp128r1,
			secp128r2 = NID_secp128r2,
			secp160k1 = NID_secp160k1,
			secp160r1 = NID_secp160r1,
			secp160r2 = NID_secp160r2,
			secp192k1 = NID_secp192k1,
			secp224k1 = NID_secp224k1,
			secp224r1 = NID_secp224r1,
			secp256k1 = NID_secp256k1,
			secp384r1 = NID_secp384r1,
			secp521r1 = NID_secp521r1,
			sect113r1 = NID_sect113r1,
			sect113r2 = NID_sect113r2,
			sect131r1 = NID_sect131r1,
			sect131r2 = NID_sect131r2,
			sect163k1 = NID_sect163k1,
			sect163r1 = NID_sect163r1,
			sect163r2 = NID_sect163r2,
			sect193r1 = NID_sect193r1,
			sect193r2 = NID_sect193r2,
			sect233k1 = NID_sect233k1,
			sect233r1 = NID_sect233r1,
			sect239k1 = NID_sect239k1,
			sect283k1 = NID_sect283k1,
			sect283r1 = NID_sect283r1,
			sect409k1 = NID_sect409k1,
			sect409r1 = NID_sect409r1,
			sect571k1 = NID_sect571k1,
			sect571r1 = NID_sect571r1
		};
		
		static constexpr curve_type DefaultCurveType = secp256k1;
		
		//return <public_key, private_key>
		static std::tuple<hex_data, hex_data> generate_key_pairs(curve_type curve_type = DefaultCurveType)
		{
			EC_KEY* ec_key = EC_KEY_new();
			EC_GROUP* ec_group = EC_GROUP_new_by_curve_name(int (curve_type));
			if (!EC_KEY_set_group(ec_key, ec_group))
			{
				EC_KEY_free(ec_key);
				EC_GROUP_free(ec_group);
				throw std::runtime_error("failed to generate keys");
			}
			if (!EC_KEY_generate_key(ec_key))
			{
				EC_KEY_free(ec_key);
				EC_GROUP_free(ec_group);
				throw std::runtime_error("failed to generate keys");
			}
			
			hex_data private_key = get_private_key(ec_key,ec_group);
			hex_data public_key = get_public_key(ec_key,ec_group);
			EC_KEY_free(ec_key);
			EC_GROUP_free(ec_group);
			return {public_key, private_key};
		}
		
		static hex_data sign(const hex_data& hash, const hex_data& private_key, curve_type curve_type = DefaultCurveType)
		{
			//set private key
			EC_KEY* ec_key = EC_KEY_new_by_curve_name(curve_type);
			BIGNUM* prv_bn = BN_new();
			const auto& private_key_str = private_key.getTextStr_lowercase();
			BN_hex2bn(&prv_bn, private_key_str.c_str());
			EC_KEY_set_private_key(ec_key, prv_bn);
			
			//generate signature
			std::vector<uint8_t> output_signature;
			ECDSA_SIG *ecdsaSig;
			ecdsaSig = ECDSA_do_sign(hash.getHexMemory().data(), int (hash.getHexMemory().size()), ec_key);
			output_signature.resize(ECDSA_size(ec_key));
			uint8_t* sig_addr = output_signature.data();
			i2d_ECDSA_SIG(ecdsaSig, &sig_addr);
			ECDSA_SIG_free(ecdsaSig);
			EC_KEY_free(ec_key);
			BN_free(prv_bn);
			return hex_data(output_signature);
		}
		
		static bool verify(const hex_data& signature, const hex_data& hash, const hex_data& public_key, curve_type curve_type = DefaultCurveType)
		{
			//set public key
			EC_KEY* ec_key = EC_KEY_new_by_curve_name(curve_type);
			EC_GROUP* ec_group = EC_GROUP_new_by_curve_name(int (curve_type));
			EC_POINT* ec_point = EC_POINT_new(ec_group);
			BN_CTX *ecctx= BN_CTX_new();
			const auto& public_key_str = public_key.getTextStr_lowercase();
			EC_POINT_hex2point(ec_group, public_key_str.c_str(), ec_point, ecctx);
			EC_KEY_set_public_key(ec_key, ec_point);
			EC_GROUP_free(ec_group);
			BN_CTX_free(ecctx);
			
			ECDSA_SIG *ecdsaSig;
			auto* signature_data = signature.getHexMemory().data();
			ecdsaSig = d2i_ECDSA_SIG(nullptr, &signature_data, int (signature.getHexMemory().size()));
			
			auto* hash_data = hash.getHexMemory().data();
			int res = ECDSA_do_verify(hash_data, int (hash.getHexMemory().size()), ecdsaSig, ec_key);
			ECDSA_SIG_free(ecdsaSig);
			EC_POINT_free(ec_point);
			EC_KEY_free(ec_key);
			return res == 1;
		}
		
	private:
		static hex_data get_public_key(EC_KEY* ec_key, EC_GROUP* ec_group)
		{
			BN_CTX *ecctx= BN_CTX_new();
			auto* pub_ = (EC_POINT*) EC_KEY_get0_public_key(ec_key);
			char* pub = EC_POINT_point2hex(ec_group, pub_, POINT_CONVERSION_HYBRID, ecctx);
			std::string output_pubkey(pub);
			BN_CTX_free(ecctx);
			delete[] pub;
			hex_data output_data(output_pubkey);
			return output_data;
		}
		
		static hex_data get_private_key(EC_KEY* ec_key, EC_GROUP* ec_group)
		{
			auto* prv_ = (BIGNUM *) EC_KEY_get0_private_key(ec_key);
			char* prv = BN_bn2hex(prv_);
			std::string output_prikey(prv);
			
			delete[] prv;
			hex_data output_data(output_prikey);
			return output_data;
		}
		
	};
}
