#ifndef TCP_H
#define TCP_H
#include "../common/io.hpp"
#include "../client/ClientState.hpp"
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

using namespace std;

int send_command_tcp(ClientState& st, string& msg, string& response);
int start_tcp(ClientState& st);
int close_tcp(ClientState& st);
int send_read_lst(ClientState& st, string& response);
int write_tcp(ClientState& st, string& msg);
int read_tcp(ClientState& st, string& msg);

#endif