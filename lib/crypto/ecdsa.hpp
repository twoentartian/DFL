////
//// Created by gzr on 23-05-21.
////
//
//#ifndef DFL_ECDSA_HPP
//#define DFL_ECDSA_HPP
//#include"ecc.hpp"
//namespace crypto {
//    template<class T>
//    class ecdsa : public ecc<T>
//    {
//    public:
//        ecdsa(T _p, T _a, T _b, T _q, T _x, T _y)
//        : ECC(_p, _a, _b, _q, _x, _y)
//        , p{ ECC.p }
//        , q{ ECC.q }
//        , P{ ECC.P }
//        {
//            ECC.P.print();
//        }
//        ecdsa(T min, T max)
//        : ECC(min, max)
//        , p{ ECC.p }
//        , q{ ECC.q }
//        , P{ ECC.P }
//        {
//            ECC.P.print();
//        }
//
//        void keygen()
//        {
//            MOD<T> = p;
//            x = rand()% (q - 1) + 2;
//            X = P * x;
//        }
//        void signatureGen(T msg)
//        {
//            MOD<T> = p;
//            k = rand()% (q - 1) + 2;
//            R = P * k;
//            r = R.x;
//            hm = sha256Hash(msg);
//            s = mod((hm + x * r) * inverse_mod(k, q), q);
//
//        }
//
//        void signatureGenKreuse(T msg)
//        {
//            MOD<T> = p;
//            R = P * k;
//            r = R.x;
//            hm = sha256Hash(msg);
//            s = mod((hm + x * r) * inverse_mod(k, q), q);
//
//        }
//
//        void retrieveKey(T s1, T s2, T hm1, T hm2, T r)
//        {
//            MOD<T> = q;
//            T key = mod(mod(s1 * hm2 - s2 * hm1) * inverse_mod((s2 - s1) * r));
//            cout << "Private key retrieved: " << key << endl;
//        }
//
//        bool verification(T msg)
//        {
//            MOD<T> = q;
//            T w = inverse_mod(s);
//            hm = sha256Hash(msg);
//            T u1 = mod(w * hm);
//            T u2 = mod(w * r);
//            MOD<T> = p;
//            auto V = (P * u1) + (X * u2);
//            MOD<T> = q;
//            if (V.x == r)
//            {
////                cout<<V.x<<endl;
////                cout<<r<<endl;
//                cout << "Verificate Successfully" << endl;
//                return true;
//            }
//            else
//            {
////                cout<<V.x<<endl;
////                cout<<r<<endl;
//                cout << "Verificate Fail" << endl;
//                return false;
//            }
//        }
//        ecc<T> ECC;
//        T &p;
//        T &q;
//        typename ecc<T>::point &P;
//        T x;
//        typename ecc<T>::point X;
//        typename ecc<T>::point R;
//        T k;
//        T r;
//        T s;
//        T hm;
//    };
//
//
//}
//#endif //DFL_ECDSA_HPP
