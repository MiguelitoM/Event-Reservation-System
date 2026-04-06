#ifndef SERVER_UDP_H
#define SERVER_UDP_H

#include <sstream>
#include "../common/io.hpp"
#include "utils.hpp"
#include <set>
#include <algorithm>
namespace fs = std::filesystem;

enum LoginResult {
    LIN_OK = 0,
    LIN_REG = 1,
    LIN_NOK = 2,
    LIN_ERR = -1
};

enum LogoutResult {
    LOU_OK = 0,
    LOU_NOK = 1,
    LOU_WRP = 2,
    LOU_UNR = 3,
    LOU_ERR = -1,
};

enum UnregisterResult {
    UNR_OK = 0,
    UNR_NOK = 1,
    UNR_WRP = 2,
    UNR_UNR = 3,
    UNR_ERR = -1,
};

enum MyeventsResult {
    RME_OK = 0,
    RME_NOK = 1,
    RME_WRP = 2,
    RME_NLG = 3,
    RME_ERR = -1,
};

enum MyreservationsResult {
    RMR_OK = 0,
    RMR_NOK = 1,
    RMR_WRP = 2,
    RMR_NLG = 3,
    RMR_ERR = -1,
};

int parse_client_msg_udp(string msg, int udp_fd, sockaddr_in udp_useraddr,socklen_t addrlen, int verbose);
int answer_udp(string command, int ret_val, int udp_fd, struct sockaddr_in udp_useraddr, socklen_t addrlen, int verbose, string uid = "", string msg = "");
int unregister(string UID, string PWD);
int logout_user(string UID, string PWD);
int login_user(string UID, string PWD);
int create_user(string UID, string PWD);
int myevents (string uid, string password, string & response);
int myreservations (string uid, string password, string & response);
void log_udp_request(const string& msg, const sockaddr_in& addr);

#endif