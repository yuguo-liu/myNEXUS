#include "server.h"

Server::Server(string ip, int port, int seed, string c_ip, int c_port) {
    // client ip and port
    client_ip = c_ip;
    client_port = c_port;

    // initialize the communication channel
    comm = new Channel(ip, port, seed);
}


Server::~Server() {

}


void Server::readSInputMatrix(int row, int col) {

}


void Server::recvHEParams() {

}