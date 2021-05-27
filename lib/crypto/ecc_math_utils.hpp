////
//// Created by gzr on 23-05-21.
////
//
//#ifndef DFL_ECC_MATH_UTILS_HPP
//#define DFL_ECC_MATH_UTILS_HPP
//#include <cstdint>
//#include <iostream>
//#include <fstream>
//#include <string>
//#include <cstdlib>
//#include <ctime>
//#include <limits>
//#include "sha256.hpp"
//using namespace std;
//namespace mathutil
//{
//
//    template<class T>
//    T sha256Hash(T message)
//    {
//        const string input = to_string(message);
//        crypto::sha256 checksum;
//        const std::string hash = checksum.digest(input);
//        T ret = static_cast<T>(stoull(string(begin(hash), begin(hash) + 4), nullptr, 16));
//        return ret;
//    }
//
//    template<class T>
//    static T MOD = 1;
//    template<class T>
//    T rd(T min, T max)
//    {
//        T r = 0;
//        for (int i =1;i<=16;++i)
//            r = (r<<16)|rand();
//        return ((r%(max-min))+min);
//    }
//
//    template<class T>
//    T mod(T x,T m = MOD<T>)
//    {
//          return ((x%m)+m)%m;
//    }
//
//    template<class T>
//    T power_mod(T base, T power, T m = MOD<T>)
//    {
//        base = mod(base, m);
//        T result = 1;
//        while (power > 0) {
//            if (power & 1)
//                result = (result * base) % m;
//            base = (base * base) % m;
//            power >>= 1;
//        }
//        return result;
//    }
//
//    template<class T>
//    T inverse_mod(T x, T m = MOD<T>)
//    {
//        return power_mod(x, m - 2, m);
//    }
//    template<class T>
//    bool millerRabinTest(T d, T n)
//    {
//        // Pick a random number in [2..n-2]
//        // Corner cases make sure that n > 4
//        T a = rd(n - 4, T(1)) + T(2);
//
//        // Compute a^d % n
//        T x = power_mod(a, d, n);
//
//        if (x == 1 || x == n - 1)
//            return true;
//
//        // Keep squaring x while one of the following doesn't
//        // happen
//        // (i)   d does not reach n-1
//        // (ii)  (x^2) % n is not 1
//        // (iii) (x^2) % n is not n-1
//        while (d != n - 1)
//        {
//            x = (x * x) % n;
//            d *= 2;
//
//            if (x == 1)
//                return false;
//            if (x == n - 1)
//                return true;
//        }
//
//        // Return composite
//        return false;
//    }
//    template<class T>
//    bool isPrime(T n, int k)
//    {
//        int lowPrimes[] =
//                { 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97
//                        , 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179
//                        , 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257, 263, 269
//                        , 271, 277, 281, 283, 293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359, 367
//                        , 373, 379, 383, 389, 397, 401, 409, 419, 421, 431, 433, 439, 443, 449, 457, 461
//                        , 463, 467, 479, 487, 491, 499, 503, 509, 521, 523, 541, 547, 557, 563, 569, 571
//                        , 577, 587, 593, 599, 601, 607, 613, 617, 619, 631, 641, 643, 647, 653, 659, 661
//                        , 673, 677, 683, 691, 701, 709, 719, 727, 733, 739, 743, 751, 757, 761, 769, 773
//                        , 787, 797, 809, 811, 821, 823, 827, 829, 839, 853, 857, 859, 863, 877, 881, 883
//                        , 887, 907, 911, 919, 929, 937, 941, 947, 953, 967, 971, 977, 983, 991, 997 };
//
//        if (n >= 3) {
//            for (int p : lowPrimes) {
//                if (n % p == 0)
//                    return false;
//            }
//        }
//
//        // Find r such that n = 2^d * r + 1 for some r >= 1
//        T d = n - 1;
//        while (d % 2 == 0)
//            d /= 2;
//
//        // Iterate given nber of 'k' times
//        for (int i = 0; i < k; i++)
//            if (millerRabinTest(d, n) == false)
//                return false;
//
//        return true;
//    }
//
//    template<class T>
//    T findRandomPrime(T min, T max, int k)
//    {
//        T prime;
//        while (1)
//        {
//            prime = rd(min, max) | T(1);
//            if (isPrime(prime, k))
//            {
//                return prime;
//            }
//        }
//    }
//
//
//}
//#endif //DFL_ECC_MATH_UTILS_HPP
