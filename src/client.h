#pragma once

#include <seal/seal.h>
#include <seal/util/uintarith.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "matrix_mul_opt.h"
#include "channel.h"
#include "pretty_print.h"

using namespace std;
using namespace seal;
using namespace seal::util;
using namespace std::chrono;

class Client {
private:
    // params for communication
    Channel* comm;

    // params for HE
    EncryptionParameters* params;       // the params of HE
    SEALContext* context;               // the context of HE
    KeyGenerator* keygen;               // the key generator of HE
    SecretKey secret_key;               // the secret key of HE (only held by client)
    PublicKey public_key;               // the public key of HE (held both sides)
    RelinKeys relin_keys;               // the relinearization key of HE (for bootstrapping)
    GaloisKeys galois_keys;             // the galois key of HE (for ciphertext rotation)
    Encryptor* encryptor;               // the encryptor of HE
    CKKSEncoder* encoder;               // the encoder of HE
    Evaluator* evaluator;               // the evaluator of HE
    Decryptor* decryptor;               // the decryptor of HE
    CKKSEvaluator* ckks_evaluator;      /* the ckks evaluator assembling context, encryptor, decryptor, 
                                               encoder, evaluator, SCALE, relin_keys, galois_keys */
    MMEvaluatorOpt* mme;                // the evaluator for matrix-matrix multiplication

    // some constants
    vector<int> MM_COEFF_MODULI = {60, 40, 60};
    double SCALE = pow(2.0, 40);

public:
    Client(string ip, int port, int seed) {
        // initialize the communication channel
        comm = new Channel(ip, port, seed);

        // initialize the HE params and evaluator
        long logN = 13;
        size_t poly_modulus_degree = 1 << logN;

        params = new EncryptionParameters(scheme_type::ckks);
        params->set_poly_modulus_degree(poly_modulus_degree);
        params->set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, MM_COEFF_MODULI));

        context = new SEALContext(*params, true, sec_level_type::none);

        keygen = new KeyGenerator(*context);
        secret_key = keygen->secret_key();
        keygen->create_public_key(public_key);
        keygen->create_relin_keys(relin_keys);

        vector<uint32_t> rots;
        for (int i = 0; i < logN; i++) {
            rots.push_back((poly_modulus_degree + exponentiate_uint(2, i)) / exponentiate_uint(2, i));
        }
        keygen->create_galois_keys(rots, galois_keys);

        encryptor = new Encryptor(*context, public_key);
        encoder = new CKKSEncoder(*context);
        evaluator = new Evaluator(*context, *encoder);
        decryptor = new Decryptor(*context, secret_key);

        ckks_evaluator = new CKKSEvaluator(*context, *encryptor, *decryptor, *encoder, *evaluator, SCALE, relin_keys, galois_keys);
        mme = new MMEvaluatorOpt(*ckks_evaluator);

        INFO_PRINT("Initialization finished.");
    }

};