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
    
    if (party == 0) {
        Client client(my_ip, my_port, SEED_CLIENT, other_ip, other_port);
        client.sendHEParams();
        client.readRandomMatrix(k, m);
        client.readCInputMatrix(k, m);
    } else if (party == 1) {
        Server server(my_ip, my_port, SEED_SERVER, other_ip, other_port);
        server.recvHEParams();
        server.readSInputMatrix(m, n);
    } else {
        ERR_PRINT("Parameter of <party> is invalid, should be 0 (client) / 1 (server), abort");
        exit(-1);
    }
}
