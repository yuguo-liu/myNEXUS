#include "server.h"


Server::Server(string ip, int port, int seed, string c_ip, int c_port, vector<MatrixInfo> &matrix_infos) {
    INFO_PRINT("Initialize the server");
    
    // client ip and port
    client_ip = c_ip;
    client_port = c_port;

    // initialize the communication channel
    comm = new Channel(ip, port, seed);

    // load in the matrix info
    for (MatrixInfo matrix_info : matrix_infos) {
        matrix_info_vec.push_back(matrix_info);
    }

    OK_PRINT("Initialization finished");
}


Server::~Server() {
    if (is_recv_HE) {
        delete comm;
        delete params;
        delete context;
        delete encryptor;
        delete encoder;
        delete evaluator;
        delete ckks_evaluator;
        delete mme;
    } else {
        delete comm;
    }
}


void Server::readSInputMatrix(int idx) {
    if (!is_recv_HE) {
        ERR_PRINT("HE params have not been received, abort");
        exit(-1);
    }

    // get matrix info
    MatrixInfo reading_matrix_info = matrix_info_vec[idx];
    int row, col;
    row = reading_matrix_info.second_row;
    col = reading_matrix_info.second_col;

    // filename of matrix
    string filename = "./data/input/matrix_server_input_m_" + to_string(row) + "_n_" + to_string(col) + ".mtx";
    INFO_PRINT("Reading random matrix: %s", filename.c_str());

    // get the matrix
    sinput_matrix = mme->readMatrix(filename, row, col);
    OK_PRINT("Finish reading random matrix: %s, size: %d x %d.", filename.c_str(), sinput_matrix.size(), sinput_matrix[0].size());
}


void Server::recvHEParams() {
    INFO_PRINT("Server is receiving HE parameters");

    // recv the HE params string
    string msg_he_params, recv_ip;
    int recv_port;
    if (!comm->recv(msg_he_params, recv_ip, recv_port)) {
        ERR_PRINT("Communication is failed");
        exit(-1);
    }

    INFO_PRINT("Server received HE parameter string");

    // parse string to params
    vector<string> parsed_he_params = split(msg_he_params, "[@@@]");
    stringstream ss_params(parsed_he_params[0]), ss_pub_key(parsed_he_params[1]), ss_relin_keys(parsed_he_params[2]), ss_galois_keys(parsed_he_params[3]);
    // cout << parsed_he_params[4] << endl;
    int poly_module_degree = stoi(parsed_he_params[4]);

    // initialize the params
    // params of HE
    params = new EncryptionParameters(scheme_type::ckks);
    params->load(ss_params);

    // context of HE
    context = new SEALContext(*params, true, sec_level_type::none);

    // public key of HE
    public_key = new PublicKey();
    public_key->load(*context, ss_pub_key);

    // relineration keys of HE
    relin_keys = new RelinKeys();
    relin_keys->load(*context, ss_relin_keys);

    // galois keys of HE
    galois_keys = new GaloisKeys();
    galois_keys->load(*context, ss_galois_keys);

    // encryptor, encoder and evaluator of HE
    encryptor = new Encryptor(*context, *public_key);
    encoder = new CKKSEncoder(*context);
    evaluator = new Evaluator(*context, *encoder);

    // ckks evaluator
    // note: decryptor is not identical to the Client, just for parameter input
    KeyGenerator keygen(*context);
    SecretKey secret_key = keygen.secret_key();
    Decryptor decryptor(*context, secret_key);

    ckks_evaluator = new CKKSEvaluator(*context, *encryptor, decryptor, *encoder, *evaluator, SCALE, *relin_keys, *galois_keys);

    // matrix-matrix evaluator
    mme = new MMEvaluatorOpt(*ckks_evaluator, poly_module_degree);

    is_recv_HE = true;
    OK_PRINT("Server's receiving HE is finished");
}


void Server::sendHECipher(vector<Ciphertext> &ciphers) {
    string s_ciphers = "";
    auto size = 0;
    int ciphers_len = ciphers.size();

    INFO_PRINT("Sending cipher of HE to client");

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
    comm->send(s_ciphers, client_ip, client_port);

    OK_PRINT("Sending cipher of HE is finished");
}


void Server::recvHECipher(vector<Ciphertext> &recv_ciphers) {
    INFO_PRINT("Receiving cipher from client");

    // receive HE string from client
    string s_ciphers, s_ip;
    int s_port;

    // p.s. server should let client know server is ready
    comm->send("ready-recv-he", client_ip, client_port);

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

    cout << 4 << endl;

    OK_PRINT("Receiving cipher is finished");
}


// note: The core of multiplication is B * [A], however, what we actually want to do is [A] * B.
//       So, we do (B^T * [A^T])T = [A] * B, meaning that the all input should be transposed
/**
 * offline phase of matrix multiplication (+ means that server should do)
 * - client sample a random matrix R
 * - client encrypt R as [R]
 * + client send matrix [R] to server
 * + server compute [R] * S (S is private matrix held by server)
 * + server return [R] * S back to client
 * - client decrypt [R] * S as R * S
 */
void Server::multiplication_offline(int idx) {
    // get row and col
    MatrixInfo reading_matrix_info = matrix_info_vec[idx];
    int row_R_T, col_R_T;
    
    // transpose, so row = col, col = row
    row_R_T = reading_matrix_info.first_col;
    col_R_T = reading_matrix_info.first_row;

    // offline phase of multiplication
    INFO_PRINT("Performing offline phase of matrix multiplication");

    // 1 - receive matrix [R] from server
    vector<Ciphertext> R_T_ct;
    recvHECipher(R_T_ct);

    // 2 - compute [R] * S (= S^T * [R^T])
    // 2.1 - transpose S
    vector<vector<double>> S_T = mme->transposeMatrix(sinput_matrix);
    
    // 2.2 - encode S as plaintext
    vector<Plaintext> S_T_pt;
    vector<Ciphertext> R_S_T_ct;
    mme->matrix_encode(S_T, S_T_pt);
    mme->matrix_cp_mul(S_T_pt, R_T_ct, col_R_T, R_S_T_ct);

    // 3 - return [R] * S back to client
    sendHECipher(R_S_T_ct);

    OK_PRINT("Offline phase of matrix multiplication is finished");
}


/**
 * online phase of matrix multiplication
 * - client compute C - R
 * + client send C - R to server
 * + server evaluate (C - R) * S
 * + server return (C - R) * S back to client
 * - client compute (C - R) * S + R * S
 */
void Server::multiplication_online(int idx) {
    INFO_PRINT("performing online phase of matrix multiplication");

    // 1 - receive C - R from client
    string C_minus_R_str, c_ip;
    int c_port;
    if (!comm->recv(C_minus_R_str, c_ip, c_port)) {
        ERR_PRINT("Communication is failed");
        exit(-1);
    }

    // 1.1 - convert matrix string to vector
    vector<vector<double>> C_minus_R = string_to_matrix(C_minus_R_str);

    // 2 - evaluate (C - R) * S
    vector<vector<double>> C_minus_R_S;
    mme->matrix_mul_in_plain(C_minus_R, sinput_matrix, C_minus_R_S);

    // 3 - return (C - R) * S back to client
    string C_minus_R_S_str = matrix_to_string(C_minus_R_S);
    comm->send(C_minus_R_S_str, client_ip, client_port);

    OK_PRINT("Offline phase of matrix multiplication is finished");
}