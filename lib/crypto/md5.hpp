
// Created by gzr on 09-05-21.

#include <stdexcept>

#if USE_OPENSSL
#include <openssl/evp.h>
#include <openssl/md5.h>
#endif

#include "./hash.hpp"
#include "./hex_data.hpp"

namespace crypto
{
    class md5 : hash{
    public:
	    static constexpr int BLOCK_SIZE = 64;
	    static constexpr int DIGEST_SIZE = 16;
    private:
	    static constexpr int S11 = 7;
		static constexpr int S12 = 12;
	    static constexpr int S13 = 17;
	    static constexpr int S14 = 22;
	    static constexpr int S21 = 5;
	    static constexpr int S22 = 9;
	    static constexpr int S23 = 14;
	    static constexpr int S24 = 20;
	    static constexpr int S31 = 4;
	    static constexpr int S32 = 11;
	    static constexpr int S33 = 16;
	    static constexpr int S34 = 23;
	    static constexpr int S41 = 6;
	    static constexpr int S42 = 10;
	    static constexpr int S43 = 15;
	    static constexpr int S44 = 21;
    	
	    inline uint32_t F(uint32_t x, uint32_t y, uint32_t z) {
		    return x&y | ~x&z;
	    }
	
	    inline uint32_t G(uint32_t x, uint32_t y, uint32_t z) {
		    return x&z | y&~z;
	    }
	
	    inline uint32_t H(uint32_t x, uint32_t y, uint32_t z) {
		    return x^y^z;
	    }
	
	    inline uint32_t I(uint32_t x, uint32_t y, uint32_t z) {
		    return y ^ (x | ~z);
	    }

		// rotate_left rotates x left n bits.
	    inline uint32_t rotate_left(uint32_t x, int n) {
		    return (x << n) | (x >> (32-n));
	    }

		// FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
		// Rotation is separate from addition to prevent recomputation.
	    inline void FF(uint32_t &a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) {
		    a = rotate_left(a+ F(b,c,d) + x + ac, s) + b;
	    }
	
	    inline void GG(uint32_t &a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) {
		    a = rotate_left(a + G(b,c,d) + x + ac, s) + b;
	    }
	
	    inline void HH(uint32_t &a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) {
		    a = rotate_left(a + H(b,c,d) + x + ac, s) + b;
	    }
	
	    inline void II(uint32_t &a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) {
		    a = rotate_left(a + I(b,c,d) + x + ac, s) + b;
	    }
	
	    uint32_t count[2];
	    uint32_t state[4];
	    uint8_t buffer[BLOCK_SIZE];
	    uint8_t digest_[DIGEST_SIZE];
	    bool finalized;
	
	    void init()
	    {
		    count[0] = 0;
		    count[1] = 0;
		    state[0] = 0x67452301;
		    state[1] = 0xefcdab89;
		    state[2] = 0x98badcfe;
		    state[3] = 0x10325476;
		    finalized = false;
	    }
	
	    void decode(uint32_t *output, const uint8_t *input,uint32_t len)
	    {
		    for(uint32_t i =0 , j=0;j<len;i++,j+=4)
		    {
			    output[i] = ((uint32_t)input[j]) | (((uint32_t)input[j+1]) << 8)|(((uint32_t)input[j+2]) << 16) | (((uint32_t)input[j+3]) << 24);
		    }
	    }
	
	    void encode(uint8_t *output, const uint32_t *input, uint32_t len)
	    {
		    for (uint32_t i = 0, j = 0; j < len; i++, j += 4) {
			    output[j] = input[i] & 0xff;
			    output[j+1] = (input[i] >> 8) & 0xff;
			    output[j+2] = (input[i] >> 16) & 0xff;
			    output[j+3] = (input[i] >> 24) & 0xff;
		    }
	    }
	
	    void transform(const uint8_t* block)
	    {
		    uint32_t a = state[0], b = state[1], c = state[2], d = state[3], x[16];
		    decode (x, block, BLOCK_SIZE);
		
		    /* Round 1 */
		    FF (a, b, c, d, x[ 0], S11, 0xd76aa478); /* 1 */
		    FF (d, a, b, c, x[ 1], S12, 0xe8c7b756); /* 2 */
		    FF (c, d, a, b, x[ 2], S13, 0x242070db); /* 3 */
		    FF (b, c, d, a, x[ 3], S14, 0xc1bdceee); /* 4 */
		    FF (a, b, c, d, x[ 4], S11, 0xf57c0faf); /* 5 */
		    FF (d, a, b, c, x[ 5], S12, 0x4787c62a); /* 6 */
		    FF (c, d, a, b, x[ 6], S13, 0xa8304613); /* 7 */
		    FF (b, c, d, a, x[ 7], S14, 0xfd469501); /* 8 */
		    FF (a, b, c, d, x[ 8], S11, 0x698098d8); /* 9 */
		    FF (d, a, b, c, x[ 9], S12, 0x8b44f7af); /* 10 */
		    FF (c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
		    FF (b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
		    FF (a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
		    FF (d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
		    FF (c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
		    FF (b, c, d, a, x[15], S14, 0x49b40821); /* 16 */
		
		    /* Round 2 */
		    GG (a, b, c, d, x[ 1], S21, 0xf61e2562); /* 17 */
		    GG (d, a, b, c, x[ 6], S22, 0xc040b340); /* 18 */
		    GG (c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
		    GG (b, c, d, a, x[ 0], S24, 0xe9b6c7aa); /* 20 */
		    GG (a, b, c, d, x[ 5], S21, 0xd62f105d); /* 21 */
		    GG (d, a, b, c, x[10], S22,  0x2441453); /* 22 */
		    GG (c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
		    GG (b, c, d, a, x[ 4], S24, 0xe7d3fbc8); /* 24 */
		    GG (a, b, c, d, x[ 9], S21, 0x21e1cde6); /* 25 */
		    GG (d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
		    GG (c, d, a, b, x[ 3], S23, 0xf4d50d87); /* 27 */
		    GG (b, c, d, a, x[ 8], S24, 0x455a14ed); /* 28 */
		    GG (a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
		    GG (d, a, b, c, x[ 2], S22, 0xfcefa3f8); /* 30 */
		    GG (c, d, a, b, x[ 7], S23, 0x676f02d9); /* 31 */
		    GG (b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */
		
		    /* Round 3 */
		    HH (a, b, c, d, x[ 5], S31, 0xfffa3942); /* 33 */
		    HH (d, a, b, c, x[ 8], S32, 0x8771f681); /* 34 */
		    HH (c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
		    HH (b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
		    HH (a, b, c, d, x[ 1], S31, 0xa4beea44); /* 37 */
		    HH (d, a, b, c, x[ 4], S32, 0x4bdecfa9); /* 38 */
		    HH (c, d, a, b, x[ 7], S33, 0xf6bb4b60); /* 39 */
		    HH (b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
		    HH (a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
		    HH (d, a, b, c, x[ 0], S32, 0xeaa127fa); /* 42 */
		    HH (c, d, a, b, x[ 3], S33, 0xd4ef3085); /* 43 */
		    HH (b, c, d, a, x[ 6], S34,  0x4881d05); /* 44 */
		    HH (a, b, c, d, x[ 9], S31, 0xd9d4d039); /* 45 */
		    HH (d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
		    HH (c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
		    HH (b, c, d, a, x[ 2], S34, 0xc4ac5665); /* 48 */
		
		    /* Round 4 */
		    II (a, b, c, d, x[ 0], S41, 0xf4292244); /* 49 */
		    II (d, a, b, c, x[ 7], S42, 0x432aff97); /* 50 */
		    II (c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
		    II (b, c, d, a, x[ 5], S44, 0xfc93a039); /* 52 */
		    II (a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
		    II (d, a, b, c, x[ 3], S42, 0x8f0ccc92); /* 54 */
		    II (c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
		    II (b, c, d, a, x[ 1], S44, 0x85845dd1); /* 56 */
		    II (a, b, c, d, x[ 8], S41, 0x6fa87e4f); /* 57 */
		    II (d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
		    II (c, d, a, b, x[ 6], S43, 0xa3014314); /* 59 */
		    II (b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
		    II (a, b, c, d, x[ 4], S41, 0xf7537e82); /* 61 */
		    II (d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
		    II (c, d, a, b, x[ 2], S43, 0x2ad7d2bb); /* 63 */
		    II (b, c, d, a, x[ 9], S44, 0xeb86d391); /* 64 */
		
		    state[0] += a;
		    state[1] += b;
		    state[2] += c;
		    state[3] += d;
		
		    // Zeroize sensitive information.
		    memset(x, 0, sizeof x);
	    }
	
	    void update(const uint8_t *input, uint32_t length)
	    {
		    uint32_t index = count[0] / 8 % BLOCK_SIZE;
		    
		    // Update number of bits
		    if ((count[0] += (length << 3)) < (length << 3))
			    count[1]++;
		    count[1] += (length >> 29);
		    uint32_t firstpart = 64 - index;
		    uint32_t i;
		    if (length >= firstpart)
		    {
			    // fill buffer first, transform
			    memcpy(&buffer[index], input, firstpart);
			    transform(buffer);
			
			    // transform chunks of BLOCK_SIZE (64 bytes)
			    for (i = firstpart; i + BLOCK_SIZE <= length; i += BLOCK_SIZE)
				    transform(&input[i]);
			
			    index = 0;
		    }
		    else
			    i = 0;
		
		    // buffer remaining input
		    memcpy(&buffer[index], &input[i], length-i);
	    }
	
	    void final()
	    {
		    static uint8_t padding[64] = {
				    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
		    };
		    
		    if(!finalized)
		    {
			    uint8_t bits[8];
			    encode(bits,count,8);
			    uint32_t index = count[0] / 8 % 64;
			    uint32_t padLen = (index < 56) ? (56 - index) : (120 - index);
			    update(padding, padLen);
			
			    // Append length (before padding)
			    update(bits, 8);
			
			    // Store state in digest
			    encode(digest_, state, 16);
			
			    // Zeroize sensitive information.
			    memset(buffer, 0, sizeof buffer);
			    memset(count, 0, sizeof count);
			
			    finalized=true;
		    }
	    }
    	
    public:
        hex_data digest(const std::string &message) override
        {
	        return digest(reinterpret_cast<const uint8_t*>(message.data()), message.size());
        }
        
        static hex_data digest_s(const std::string &message)
        {
	        return digest_s(reinterpret_cast<const uint8_t*>(message.data()), message.size());
        }
	
	    hex_data digest(const uint8_t* data, size_t size) override
	    {
		    init();
		    update(data, size);
		    final();
		    if (!finalized)
		    {
			    //Error
			    throw std::logic_error("fail to calculate md5");
		    }
		    return hex_data(digest_, 16);
	    }
	
	    static hex_data digest_s(const uint8_t* data, size_t size)
	    {
	    	md5 ctx;
		    ctx.init();
		    ctx.update(data, size);
		    ctx.final();
		    if (!ctx.finalized)
		    {
			    //Error
			    throw std::logic_error("fail to calculate md5");
		    }
		    return hex_data(ctx.digest_, 16);
	    }

#if USE_OPENSSL
        hex_data digest_openssl(const std::string& message) override
        {
	        std::string hash;
	        hash.resize(128 / 8);
	        MD5((const unsigned char *)message.c_str(), message.size(), (unsigned char *)hash.c_str());
	        hex_data output(hash.c_str(), DIGEST_SIZE);
	        return output;
        }
#endif

    };
}
