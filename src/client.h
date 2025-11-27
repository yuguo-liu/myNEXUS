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
    Channel* comm;                      // client's communication
    string server_ip;                   // ip of server
    int server_port;                    // port of server

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

    // matrix
    vector<vector<double>> random_matrix;       // the matrix to mask the `client matrix`
    vector<vector<double>> cinput_matrix;       // the matrix of private input

    // some constants
    vector<int> MM_COEFF_MODULI = {60, 40, 60};
    double SCALE = pow(2.0, 40);

public:
    // initial the client
    Client(string ip, int port, int seed, string s_ip, int s_port);
    // clean up when client is deleted
    ~Client();
    // load the random matrix
    void readRandomMatrix(int row, int col);
    // load the input matrix of client
    void readCInputMatrix(int row, int col);
    // send HE params to server
    void sendHEParams();
};