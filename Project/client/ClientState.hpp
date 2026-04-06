#ifndef STATE_H
#define STATE_H

#include <netdb.h>
#include <string>

using namespace std;

struct ClientState {
    int udp_fd = -1;
    int tcp_fd = -1;
    char *port;
    char *domain;
    addrinfo* server_addr_udp = nullptr;
    addrinfo* server_addr_tcp = nullptr;
    string username = "";
    string password = "";
    bool logged_in = false;
};

#endif