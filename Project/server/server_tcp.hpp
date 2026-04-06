#ifndef SERVER_TCP_H
#define SERVER_TCP_H

#include <sstream>
#include "../common/io.hpp"
#include "utils.hpp"
#include "CreState.hpp"
#include <map>
#include <set>
#define SESSION_TIMEOUT 60*15

enum CreateResult {
    CRE_OK = 0,
    CRE_NOK = 1,
    CRE_WRP = 2,
    CRE_NLG = 3,
    CRE_ERR = -1,
};

enum CloseResult {
    RCL_OK = 0,
    RCL_NOK = 1,
    RCL_NLG = 2,
    RCL_NOE = 3,
    RCL_EOW = 4,
    RCL_SLD = 5,
    RCL_PST = 6,
    RCL_CLO = 7,
    RCL_ERR = -1,
};

enum ReserveResult {
    RRI_ACC = 0,
    RRI_NOK = 1,
    RRI_NLG = 2,
    RRI_CLS = 3,
    RRI_REJ = 4,
    RRI_SLD = 5,
    RRI_PST = 6,
    RRI_WRP = 7,
    RRI_ERR = -1,
};

enum ChangePassResult {
    RCP_OK = 0,
    RCP_NOK = 1,
    RCP_NLG = 2,
    RCP_NID = 3,
    RCP_ERR = -1,
};

enum ShowResult {
    RSE_OK = 0,
    RSE_NOK = 1,
    RSE_ERR = -1,
};

enum ListResult {
    RLS_OK = 0,
    RLS_NOK = 1,
    RLS_ERR = -1,
};

int parse_client_msg_tcp(int fd, string msg, int verbose, const sockaddr_in& addr);
int try_to_parse_create(int fd, string& msg, CreState &state, int verbose, const sockaddr_in& addr);
int start_create_event(CreState& state);
int write_event(CreState &state, const char* msg, int amount_to_write);
int close_event(string uid, string password, string eid);
int reserve(string uid, string password, string eid, int number_of_seats, int& remaining);
int list(int fd,int verbose, const sockaddr_in& addr);
int answer_create(int ret_val, int fd, CreState& state, int verbose, const sockaddr_in& addr);
int answer_close(int ret_val, int fd, int verbose, const sockaddr_in& addr, string uid = "");
void disconnect_tcp(int fd, map<int, CreState> &states, fd_set& inputs, map<int, string>& tcp_buffers,
                    map<int, time_t>& connection_start, map<int, sockaddr_in>& tcp_clients);
int answer_reserve(int ret_val, int fd, int remaining, int verbose, const sockaddr_in& addr, string uid = "");
int changePass(string uid, string old_pass, string new_pass);
int answer_changePass(int ret_val, int fd, int verbose, const sockaddr_in& addr, string uid = "");
int answer_ERR(int fd, int verbose, const sockaddr_in& addr);
int show(int fd, string eid, int verbose, const sockaddr_in& addr);
bool time_out(int fd, map<int, time_t>& connection_start);
void log_tcp_request(const string& msg, const sockaddr_in& addr);
void log_answer(string answer, const sockaddr_in& addr, string uid = "");


#endif