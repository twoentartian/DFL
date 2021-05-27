//
// Created by gzr on 17-05-21.
//

#ifndef DFL_ECC_HPP
#define DFL_ECC_HPP
#include <cmath>
#include "hash.hpp"
#include <unordered_set>
#include "ecc_math_utils.hpp"
using namespace mathutil;
namespace crypto{
    template<class T>
    class ecc {
    public:
        T a;
        T b;
        T p;
        T q;
        T min;
        T max;

        ecc() {}

        ecc(T _p, T _a, T _b, T _q, T _x, T _y) {
            p = _p;
            a = _a;
            b = _b;
            q = _q;
            P.x = _x;
            P.y = _y;
            P.a = _a;
            P.b = _b;
            P.p = _p;
        }

        ecc(T min_, T max_) {
            min = min_;
            max = max_;
            generate(min, max);
        }

        struct point {
            point() {}

            point(T x, T y) : x{x}, y{y} {}

            point(T a, T b, T p, T x, T y) : a{a},b{b},p{p} ,x{x}, y{y} {}
            T a;
            T b;
            T p ;
            T x;
            T y;

            bool operator==(const point &rhs) const {
                return x == rhs.x && y == rhs.y;
            }

            point operator+(point p2) {
                typename ecc<T>::point &p1 = *this;

                if (p1 == p2)
                    return p1.doubl(p2.a,p2.b,p2.p);

                if (p1.x == p2.x) {
                    return {0, 0};
                }

                if (p1.x == 0 && p1.y == 0)
                    return p2;

                if (p2.x == 0 && p2.y == 0)
                    return p1;

                T s = mod(mod(p2.y - p1.y) * inverse_mod(p2.x - p1.x));

                T x3 = mod(s * s - p1.x - p2.x);

                return {p2.a,p2.b,p2.p,x3, mod(s * (p1.x - x3) - p1.y)};
            }

            point operator*(T k) {
                if (k == 0)
                    return typename ecc<T>::point(0, 0);
                else if (k == 1)
                    return *this;
                else if (k % 2 == 1)
                    return *this + *this * (k - 1);
                else
                    return this->doubl(this->a,this->b,this->p) * (k / 2);
            }

            point doubl(T a, T b, T p) const {
                if (y == 0) {
                    return {0, 0};
                }

                T s = mod(mod(3 * x * x + a) * inverse_mod(2 * y));

                T x3 = mod(s * s - x - x);
                return {a,b,p,x3, mod(s * (x - x3) - y)};
            }

            void print() const {
                std::cout << "point is (" << x << ", " << y << ")" << std::endl;
            }
        };
        point P;
        struct pointHash {
            std::size_t operator()(point const &pt) const noexcept {
                return std::hash<long long>{}(pt.x ^ (pt.y << 1));
            }
        };

        using CyclicGroup = std::unordered_set<point, pointHash>;

        void generate(T min, T max) {
            while (1) {
                p = findRandomPrime(min, max, 1 << 15);
                MOD<T> = p;

                P.x = rd(min,max) % p;
                P.y = rd(min,max) %p;
                a = rd(min,max) % p;
                b = mod(P.y * P.y - P.x * P.x * P.x - P.x * a);
                P.a = a;
                P.b = b;
                P.p = p;
                if (mod(4 * a * a * a + 27 * b * b) != 0) {
                    createCyclicGroup();
                    auto sqrtp = sqrt(p);
                    if (isPrime(q, 1 << 15) && p + 1 - 2 * sqrtp < q && p + 1 + 2 * sqrtp > q)
                        break;
                }
            }
            return;
        }

        T createCyclicGroup() {
            typename ecc<T>::point ppt = P;
            const typename ecc<T>::point infi(0, 0);
            E.clear();

            while (1) {
                E.insert(ppt);
                if (ppt == infi)
                    break;

                ppt = ppt + P;
            }
            q = E.size();

            return q;
        }

        CyclicGroup E;

        void printInfo() {
            cout << "ECC params: a = " << a
                 << ", b = " << b
                 << ", p = " << p << endl
                 << "Start point: (" << P.x
                 << ", " << P.y
                 << ")" << endl
                 << "|E| = " << q << endl;
        }

    };
}




#endif //DFL_ECC_HPP




