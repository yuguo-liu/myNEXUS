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
#include "utils.h"
#include "matrix_info.h"

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
    
    vector<vector<double>> interval_matrix;     // the matrix intervally generated to help matrix multiplication

    vector<vector<double>> result_matrix;       // the result matrix of multiplication

    // matrix info
    vector<MatrixInfo> matrix_info_vec;         // the vector of matrix information

    // some constants
    vector<int> MM_COEFF_MODULI = {60, 40, 60};
    double SCALE = pow(2.0, 40);
    size_t poly_modulus_degree;

public:
    // initial the client
    Client(string ip, int port, int seed, string s_ip, int s_port, vector<MatrixInfo> &matrix_infos);
    // clean up when client is deleted
    ~Client();
    // load the random matrix
    void readRandomMatrix(int idx);
    // load the input matrix of client
    void readCInputMatrix(int idx);
    // send HE params to server
    void sendHEParams();
    // send HE cipher to server
    void sendHECipher(vector<Ciphertext> &ciphers);
    // recv HE cipher from server
    void recvHECipher(vector<Ciphertext> &recv_ciphers);
    // offline phase of multiplication
    void multiplication_offline(int idx);
    // online phase of multiplication
    void multiplication_online(int idx);
    // clear matrix
    void clear_matrix();
    // print some elements in the result matrix
    void glanceResultMatrix(int rows, int cols);
    // print some elements in the interval matrix
    void glanceIntervalMatrix(int rows, int cols);
};