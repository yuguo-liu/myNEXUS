#include <seal/seal.h>
#include <seal/util/uintarith.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "matrix_mul_opt.h"

using namespace std;
using namespace seal;
using namespace seal::util;
using namespace std::chrono;

vector<int> MM_COEFF_MODULI = {60, 40, 60};
double SCALE = pow(2.0, 40);

void test_matrix_mul(vector<vector<double>> &a, vector<vector<double>> &b, MMEvaluatorOpt &mme) {
    vector<vector<double>> res;
    mme.matrix_mul_in_plain(a, b, res);

    for (vector<double> v : res) {
        for (double e : v) {
            cout << e << " ";
        }
        cout << endl;
    }
}

int main() {
    long logN = 13;
    size_t poly_modulus_degree = 1 << logN;

    EncryptionParameters parms(scheme_type::ckks);
    parms.set_poly_modulus_degree(poly_modulus_degree);
    parms.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, MM_COEFF_MODULI));

    SEALContext context(parms, true, sec_level_type::none);

    KeyGenerator keygen(context);
    SecretKey secret_key = keygen.secret_key();
    PublicKey public_key;
    keygen.create_public_key(public_key);
    RelinKeys relin_keys;
    keygen.create_relin_keys(relin_keys);
    GaloisKeys galois_keys;

    std::vector<std::uint32_t> rots;
    for (int i = 0; i < logN; i++) {
        rots.push_back((poly_modulus_degree + exponentiate_uint(2, i)) / exponentiate_uint(2, i));
    }
    keygen.create_galois_keys(rots, galois_keys);

    Encryptor encryptor(context, public_key);
    CKKSEncoder encoder(context);
    Evaluator evaluator(context, encoder);
    Decryptor decryptor(context, secret_key);

    CKKSEvaluator ckks_evaluator(context, encryptor, decryptor, encoder, evaluator, SCALE, relin_keys, galois_keys);
    MMEvaluatorOpt mme(ckks_evaluator);

    // test for 
    vector<vector<double>> A = {
        {1, 2, 3},
        {4, 5, 6}
    };
    
    vector<vector<double>> B = {
        {7, 8},
        {9, 10},
        {11, 12}
    };

    test_matrix_mul(A, B, mme);
}
