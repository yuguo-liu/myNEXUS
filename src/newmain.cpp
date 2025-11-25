#include "client.h"
#include "server.h"

#define SEED 42

int main(int argc, char** argv) {
    // get the parameter
    int k, m, n, party, port;            // matrix #1 (k x m), matrix #2 (m x n)
    string ip;
    if (argc < 7) {
        cerr << "[Err] Invalid input\n Valid input should be ./newmain <k> <m> <n> <party> <ip> <port>" << endl;
        exit(-1);
    }

    k = stoi(argv[1]);
    m = stoi(argv[2]);
    n = stoi(argv[3]);
    party = stoi(argv[4]);
    ip = argv[5];
    port = stoi(argv[6]);

    INFO_PRINT("Get the parameter from party %d: matrix #1 (%d x %d), matrix #2 (%d x %d)", party, k, m, m, n);    
    
    if (party == 0) {
        Client client(ip, port, SEED);
    }
}
