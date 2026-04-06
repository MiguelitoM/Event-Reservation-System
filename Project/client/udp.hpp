#ifndef UDP_H
#define UDP_H
#include "../common/io.hpp"
#include "ClientState.hpp"
#include "../common/tcp.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#define RESPONSE_SIZE 7
#define RESPONSE_SIZE_MYEVENTS 7000
#define RESPONSE_SIZE_MYRESERVATIONS 7000
int start_udp(ClientState& st, char* domain, char* s_port);
int send_command_udp(ClientState& st, string& msg, char* response);
int send_command_udp_myevents(ClientState& st, string& msg, char* response);
int send_command_udp_myreservations(ClientState& st, string& msg, char* response);

#endif