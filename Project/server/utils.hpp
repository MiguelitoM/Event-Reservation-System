#ifndef UTILS_H
#define UTILS_H
#define USER_PATH "users"
#define EVENT_PATH "events"
#define EVENT_COUNT "last_event"
#include "../common/io.hpp"

namespace fs = std::filesystem;

void destroy_all_events();
fs::path user_dir(const std::string& uid);
bool check_password(string uid, string password);
fs::path event_dir(const std::string& parse_eid);
fs::path event_description_dir(const std::string& eid);
fs::path event_description_path(const std::string& eid);
fs::path password_path(const std::string& uid);
fs::path user_reservations_dir(const std::string& uid);
fs::path user_events_dir(const std::string& uid);
fs::path logged_in_path(const std::string& uid);
fs::path event_count_path();
bool is_closed(string uid);
fs::path event_details_path(string eid);
fs::path event_reservations_dir(string eid);
fs::path event_total_res_path(string eid);
bool is_logged_in(string uid);
bool does_user_exist(string uid);
bool does_event_exist(string eid);
string create_event_dirs();
void destroy_event(string eid);
string get_event_creator(string eid);
int get_event_attendance(string eid);
int get_event_sold(string eid);
string get_event_date(string eid);
string get_event_hour(string eid);
string get_event_name(string eid);
int get_event_state(string eid);
int set_event_sold(string eid, int tickets);
bool is_event(fs::path event_dir);
#endif