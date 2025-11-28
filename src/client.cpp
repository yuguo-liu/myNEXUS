#include "client.h"


Client::Client(string ip, int port, int seed, string s_ip, int s_port) {
    INFO_PRINT("Initialize the client");

    // server ip and port
    server_ip = s_ip;
    server_port = s_port;
    
    // initialize the communication channel
    comm = new Channel(ip, port, seed);

    // initialize the HE params and evaluator
    long logN = 13;
    poly_modulus_degree = 1 << logN;

    // params of HE
    params = new EncryptionParameters(scheme_type::ckks);
    params->set_poly_modulus_degree(poly_modulus_degree);
    params->set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, MM_COEFF_MODULI));

    // context of HE
    context = new SEALContext(*params, true, sec_level_type::none);

    // secret key, relineration keys and public key of HE
    keygen = new KeyGenerator(*context);
    secret_key = keygen->secret_key();
    keygen->create_public_key(public_key);
    keygen->create_relin_keys(relin_keys);

    vector<uint32_t> rots;
    for (int i = 0; i < logN; i++) {
        rots.push_back((poly_modulus_degree + exponentiate_uint(2, i)) / exponentiate_uint(2, i));
    }
    keygen->create_galois_keys(rots, galois_keys);

    // encryptor, encoder, evaluator and decryptor of HE
    encryptor = new Encryptor(*context, public_key);
    encoder = new CKKSEncoder(*context);
    evaluator = new Evaluator(*context, *encoder);
    decryptor = new Decryptor(*context, secret_key);

    // ckks evaluator
    ckks_evaluator = new CKKSEvaluator(*context, *encryptor, *decryptor, *encoder, *evaluator, SCALE, relin_keys, galois_keys);
    
    // matrix-matrix evaluator
    mme = new MMEvaluatorOpt(*ckks_evaluator, poly_modulus_degree);

    OK_PRINT("Initialization finished");
}


Client::~Client() {
    delete comm;
    delete params;
    delete context;
    delete keygen;
    delete encryptor;
    delete encoder;
    delete evaluator;
    delete decryptor;
    delete ckks_evaluator;
    delete mme;
}


void Client::readRandomMatrix(int row, int col) {
    // filename of matrix
    string filename = "./data/input/matrix_random_input_k_" + to_string(row) + "_m_" + to_string(col) + ".mtx";
    INFO_PRINT("Reading random matrix: %s", filename.c_str());

    // get the matrix
    random_matrix = mme->readMatrix(filename, row, col);
    OK_PRINT("Finish reading random matrix: %s, size: %d x %d.", filename.c_str(), random_matrix.size(), random_matrix[0].size());
}


void Client::readCInputMatrix(int row, int col) {
    // filename of matrix
    string filename = "./data/input/matrix_client_input_k_" + to_string(row) + "_m_" + to_string(col) + ".mtx";
    INFO_PRINT("Reading random matrix: %s", filename.c_str());

    // get the matrix
    cinput_matrix = mme->readMatrix(filename, row, col);
    OK_PRINT("Finish reading random matrix: %s, size: %d x %d.", filename.c_str(), cinput_matrix.size(), cinput_matrix[0].size());
}


void Client::sendHEParams() {
    stringstream ss_params, ss_pub_key, ss_relin_keys, ss_galois_keys;

    // serialize the params of HE
    INFO_PRINT("Client is parsing the parameters of HE");
    params->save(ss_params);
    public_key.save(ss_pub_key);
    relin_keys.save(ss_relin_keys);
    galois_keys.save(ss_galois_keys);

    // send it to server
    string msg_he_params = ss_params.str() + "[@]" + \
                            ss_pub_key.str() + "[@]" + \
                            ss_relin_keys.str() + "[@]" + \
                            ss_galois_keys.str() + "[@]" + \
                            to_string(poly_modulus_degree);
    comm->send(msg_he_params, server_ip, server_port);

    OK_PRINT("Sending HE params is finished");
}


void Client::sendHECipher(vector<Ciphertext> &ciphers) {
    string s_ciphers = "";
    auto size = 0;
    int ciphers_len = ciphers.size();

    INFO_PRINT("Sending cipher of HE to server");

    // convert ciphertext to stringstream
    for (int i = 0; i < ciphers_len; i++) {
        stringstream ss_cipher;
        size += ciphers[i].save(ss_cipher);
        
        if (i < ciphers_len - 1) {
            s_ciphers += ss_cipher.str() + "[@]";
        } else {
            s_ciphers += ss_cipher.str();
        }
    }

    // send string to server
    INFO_PRINT("Ready to send HE cipher with %f MB", size / 1024.0 / 1024.0);
    comm->send(s_ciphers, server_ip, server_port);

    OK_PRINT("Sending cipher of HE is finished");
}


void Client::recvHECipher(vector<Ciphertext> &recv_ciphers) {
    INFO_PRINT("Receiving cipher from server");

    // receive HE string from server
    string s_ciphers, s_ip;
    int s_port;
    
    if (!comm->recv(s_ciphers, s_ip, s_port)) {
        ERR_PRINT("Communication is failed");
        exit(-1);
    }

    // parse the string to ciphertexts
    vector<string> s_ciphers_vec = split(s_ciphers, "[@]");

    for (string s : s_ciphers_vec) {
        stringstream ss(s);
        Ciphertext single_cipher;

        single_cipher.load(*context, ss);
        recv_ciphers.push_back(single_cipher);
    }

    OK_PRINT("Receiving cipher is finished");
}

