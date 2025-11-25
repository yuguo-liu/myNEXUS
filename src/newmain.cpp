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


int main(int argc, char** argv) {
    // get the parameter
    int k, m, n, party;            // matrix #1 (k x m), matrix #2 (m x n)
    if (argc < 5) {
        cerr << "[Err] Invalid input\n Valid input should be ./newmain <k> <m> <n>" << endl;
        exit(-1);
    }

    k = stoi(argv[1]);
    m = stoi(argv[2]);
    n = stoi(argv[3]);
    party = stoi(argv[4]);

    cout << "[Info] Get the parameter from party " << to_string(party) << ": matrix #1 (" \
            << to_string(k) << " x " << to_string(m) << \
            "), matrix #2 (" << to_string(m) << " x " << to_string(n) << ")" << endl;

    // setup for ckks
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

    
}
