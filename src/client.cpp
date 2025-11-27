#include "client.h"


Client::Client(string ip, int port, int seed, string s_ip, int s_port) {
    // server ip and port
    server_ip = s_ip;
    server_port = s_port;
    
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

    OK_PRINT("Initialization finished.");
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
    string filename = "../data/input/matrix_random_input_k_" + to_string(row) + "_m_" + to_string(col) + ".mtx";
    INFO_PRINT("Reading random matrix: %s", filename.c_str());

    // get the matrix
    random_matrix = mme->readMatrix(filename, row, col);
    OK_PRINT("Finish reading random matrix: %s, size: %d x %d.", filename.c_str(), random_matrix.size(), random_matrix[0].size());
}


void Client::readCInputMatrix(int row, int col) {
    // filename of matrix
    string filename = "../data/input/matrix_client_input_k_" + to_string(row) + "_m_" + to_string(col) + ".mtx";
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
                            ss_galois_keys.str();
    comm->send(msg_he_params, server_ip, server_port);

    OK_PRINT("Client's sending is not finished");
}
