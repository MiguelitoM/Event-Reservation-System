#ifndef IO_H
#define IO_H
#define BUFFSIZE 8192

#include <filesystem> 
#include <vector>
#include "io.hpp"
#include <arpa/inet.h>
#include <fstream>
#include <iostream>
#include <netdb.h>
#include <stdio.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#define MAX_FILESIZE 10000000

using namespace std;

namespace fs = std::filesystem;

int parse_login_logout(string uid, string password);
int parse_uid(string uid);
int parse_password(string password);
int safe_read(int fd, string& out);
int safe_write(int fd, string& msg);
int parse_date(string date);
int parse_hour(string hour);
int parse_name(string name);
int parse_filename(string filename);
int parse_attendance(string attendance);
int parse_eid(string eid);
int parse_value_reserved(string value);
int parse_seats_reserved(string value);
int parse_hour_with_seconds(string value);
int parse_filesize(string filesize);
bool is_event_past(string& date, string& hour);
int parse_reserve_number(string amount);
int send_file(int fd, fs::path file_path);

#endif 