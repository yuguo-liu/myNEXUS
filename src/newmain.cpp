#include "client.h"
#include "server.h"

#define SEED_CLIENT 42
#define SEED_SERVER 43

int main(int argc, char** argv) {
    // get the parameter
    int k, m, n, party, my_port, other_port;            // matrix #1 (k x m), matrix #2 (m x n)
    string my_ip, other_ip;
    if (argc < 9) {
        cerr << "[Err] Invalid input\n Valid input should be ./newmain <k> <m> <n> <party> <ip> <port> <t_ip> <t_port>, abort" << endl;
        exit(-1);
    }

    k = stoi(argv[1]);
    m = stoi(argv[2]);
    n = stoi(argv[3]);
    party = stoi(argv[4]);
    my_ip = argv[5];
    my_port = stoi(argv[6]);
    other_ip = argv[7];
    other_port = stoi(argv[8]);

    INFO_PRINT("Get the parameter from party %d: matrix #1 (%d x %d), matrix #2 (%d x %d)", party, k, m, m, n);    
    INFO_PRINT("my ip: %s:%d", my_ip.c_str(), my_port);

    // generate matrix info
    MatrixInfo matrix_info(0, k, m, n);

    vector<MatrixInfo> matrix_info_vec;
    matrix_info_vec.push_back(matrix_info);

    if (party == 0) {
        Client client(my_ip, my_port, SEED_CLIENT, other_ip, other_port, matrix_info_vec);
        client.sendHEParams();
        client.readRandomMatrix(0);
        client.readCInputMatrix(0);
        client.multiplication_offline(0);
        client.multiplication_online(0);
        client.glanceResultMatrix(10, 10);
        client.glanceIntervalMatrix(10, 10);
    } else if (party == 1) {
        Server server(my_ip, my_port, SEED_SERVER, other_ip, other_port, matrix_info_vec);
        server.recvHEParams();
        server.readSInputMatrix(0);
        server.multiplication_offline(0);
        server.multiplication_online(0);
    } else {
        ERR_PRINT("Parameter of <party> is invalid, should be 0 (client) / 1 (server), abort");
        exit(-1);
    }
}
