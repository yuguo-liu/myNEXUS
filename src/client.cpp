#include "client.h"


Client::Client(string ip, int port, int seed, string s_ip, int s_port, vector<MatrixInfo> &matrix_infos) {
    INFO_PRINT("Initialize the client");

    // server ip and port
    server_ip = s_ip;
    server_port = s_port;
    
    // initialize the communication channel
    comm = new Channel(ip, port, seed);

    // load in the matrix info
    for (MatrixInfo matrix_info : matrix_infos) {
        matrix_info_vec.push_back(matrix_info);
    }

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


void Client::readRandomMatrix(int idx) {
    // get matrix info
    MatrixInfo reading_matrix_info = matrix_info_vec[idx];
    int row, col;
    row = reading_matrix_info.first_row;
    col = reading_matrix_info.first_col;

    // filename of matrix
    string filename = "./data/input/matrix_random_input_k_" + to_string(row) + "_m_" + to_string(col) + ".mtx";
    INFO_PRINT("Reading random matrix: %s", filename.c_str());

    // get the matrix
    random_matrix = mme->readMatrix(filename, row, col);
    OK_PRINT("Finish reading random matrix: %s, size: %d x %d.", filename.c_str(), random_matrix.size(), random_matrix[0].size());
}


void Client::readCInputMatrix(int idx) {
    // get matrix info
    MatrixInfo reading_matrix_info = matrix_info_vec[idx];
    int row, col;
    row = reading_matrix_info.first_row;
    col = reading_matrix_info.first_col;

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
    string msg_he_params = ss_params.str() + "[@@@]" + \
                            ss_pub_key.str() + "[@@@]" + \
                            ss_relin_keys.str() + "[@@@]" + \
                            ss_galois_keys.str() + "[@@@]" + \
                            to_string(poly_modulus_degree);
    comm->send(msg_he_params, server_ip, server_port);

    OK_PRINT("Sending HE params is finished");
}


void Client::sendHECipher(vector<Ciphertext> &ciphers) {
    string s_ciphers = "";
    auto size = 0;
    int ciphers_len = ciphers.size();

    INFO_PRINT("Sending cipher of HE to server");

    // p.s. client should know server is ready
    string ready_msg, s_ip;
    int s_port;
    if (!comm->recv(ready_msg, s_ip, s_port)) {
        ERR_PRINT("Communication is failed");
        exit(-1);
    }

    if (ready_msg != "ready-recv-he") {
        ERR_PRINT("Invalid label, abort");
        exit(-1);
    }

    // convert ciphertext to stringstream
    for (int i = 0; i < ciphers_len; i++) {
        stringstream ss_cipher;
        size += ciphers[i].save(ss_cipher);
        
        if (i < ciphers_len - 1) {
            s_ciphers += ss_cipher.str() + "[@@@]";
        } else {
            s_ciphers += ss_cipher.str();
        }
    }

    // send string to server
    INFO_PRINT("Ready to send HE cipher with %f MB", size / 1024.0 / 1024.0);
    // cout << s_ciphers << endl;
    comm->send(s_ciphers, server_ip, server_port);

    OK_PRINT("Sending cipher of HE is finished");
}


void Client::recvHECipher(vector<Ciphertext> &recv_ciphers) {
    INFO_PRINT("Receiving cipher from server");

    // receive HE string from server
    string s_ciphers, s_ip;
    int s_port;
    
    // p.s. server should let client know server is ready
    comm->send("ready-recv-he", server_ip, server_port);

    if (!comm->recv(s_ciphers, s_ip, s_port)) {
        ERR_PRINT("Communication is failed");
        exit(-1);
    }

    // parse the string to ciphertexts
    vector<string> s_ciphers_vec = split(s_ciphers, "[@@@]");

    for (string s : s_ciphers_vec) {
        stringstream ss(s);
        Ciphertext single_cipher;

        single_cipher.load(*context, ss);
        recv_ciphers.push_back(single_cipher);
    }

    OK_PRINT("Receiving cipher is finished");
}


// note: The core of multiplication is B * [A], however, what we actually want to do is [A] * B.
//       So, we do (B^T * [A^T])T = [A] * B, meaning that the all input should be transposed
/**
 * offline phase of matrix multiplication (+ means that client should do)
 * + client sample a random matrix R
 * + client encrypt R as [R]
 * + client send matrix [R] to server
 * - server compute [R] * S (S is private matrix held by server)
 * + server return [R] * S back to client
 * + client decrypt [R] * S as R * S
 */
void Client::multiplication_offline(int idx) {
    // offline phase of multiplication
    INFO_PRINT("Performing offline phase of matrix multiplication");

    // get matrix info
    MatrixInfo m_info = matrix_info_vec[idx];
    int res_row = m_info.first_row;
    int res_col = m_info.second_col;

    // 1 - sample a random matrix R
    // note: read the random matrix from file, stored in random_matrix
    
    // 2 - encrypt R as [R]
    vector<Ciphertext> R_T_ct;
    vector<vector<double>> R_T = mme->transposeMatrix(random_matrix);
    mme->matrix_encrypt(R_T, R_T_ct);

    // 3 - send matrix [R] to server
    sendHECipher(R_T_ct);

    // 4 - receive [R] * S from server
    vector<Ciphertext> R_S_T_ct;
    recvHECipher(R_S_T_ct);

    // 5 - decrypt [R] * S as R * S
    // note: R_S_T is transposed, so the row and col should swap
    vector<vector<double>> R_S_T(res_col, vector<double>(res_row, 0.0));
    mme->matrix_decrypt(R_S_T_ct, R_S_T);
    // 5.1 - tranpose to get R * S
    interval_matrix = mme->transposeMatrix(R_S_T);

    OK_PRINT("Offline phase of matrix multiplication is finished");
}

/**
 * online phase of matrix multiplication
 * + client compute C - R
 * + client send C - R to server
 * - server evaluate (C - R) * S
 * + server return (C - R) * S back to client
 * + client compute (C - R) * S + R * S
 */
void Client::multiplication_online(int idx) {
    INFO_PRINT("Performing online phase of matrix multiplication");

    // 1 - compute C - R
    vector<vector<double>> C_minus_R;
    mme->matrix_sub_in_plain(cinput_matrix, random_matrix, C_minus_R);

    // 2 - send C - R to server
    string C_minus_R_str = matrix_to_string(C_minus_R);
    comm->send(C_minus_R_str, server_ip, server_port);

    // 3 - receive (C - R) * S from server
    string C_minus_R_S_str, c_ip;
    int c_port;
    if (!comm->recv(C_minus_R_S_str, c_ip, c_port)) {
        ERR_PRINT("Communication is failed");
        exit(-1);
    }

    vector<vector<double>> C_minus_R_S = string_to_matrix(C_minus_R_S_str);

    // 4 - compute (C - R) * S + R * S
    INFO_PRINT("C_minus_R_S:     %d x %d", C_minus_R_S.size(), C_minus_R_S[0].size());
    INFO_PRINT("interval_matrix: %d x %d", interval_matrix.size(), interval_matrix[0].size());
    mme->matrix_add_in_plain(C_minus_R_S, interval_matrix, result_matrix);
}


void Client::clear_matrix() {
    for (vector<double> im : interval_matrix) im.clear();
    interval_matrix.clear();
    
    for (vector<double> rm : result_matrix) rm.clear();
    result_matrix.clear();
}

/**
 * print some elements in matrix
 * - rows: numbers of row to print (0 means print all)
 * - cols: numbers of col to print (0 means print all)
 */
void Client::glanceResultMatrix(int rows, int cols) {
    if (rows > result_matrix.size() || cols > result_matrix.size()) {
        ERR_PRINT("Printing matrix is out of range, abort");
        exit(-1);
    }

    int print_rows = (rows == 0) ? result_matrix.size() : rows;
    int print_cols = (cols == 0) ? result_matrix[0].size() : cols;

    DEBUG_PRINT("Some elements in result matrix");
    for (int i = 0; i < print_rows; i++) {
        cout << "row #" << i << ": ";
        for (int j = 0; j < print_cols; j++) {
            cout << result_matrix[i][j] << " ";
        }
        cout << endl;
    }
}

/**
 * print some elements in matrix
 * - rows: numbers of row to print (0 means print all)
 * - cols: numbers of col to print (0 means print all)
 */
void Client::glanceIntervalMatrix(int rows, int cols) {
    if (rows > interval_matrix.size() || cols > interval_matrix.size()) {
        ERR_PRINT("Printing matrix is out of range, abort");
        exit(-1);
    }

    int print_rows = (rows == 0) ? interval_matrix.size() : rows;
    int print_cols = (cols == 0) ? interval_matrix[0].size() : cols;

    DEBUG_PRINT("Some elements in interval matrix");
    for (int i = 0; i < print_rows; i++) {
        cout << "row #" << i << ": ";
        for (int j = 0; j < print_cols; j++) {
            cout << interval_matrix[i][j] << " ";
        }
        cout << endl;
    }
}